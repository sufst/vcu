/***************************************************************************
 * @file   sensor_thread.c
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2021-11-30
 * @brief  Sensor thread
 ***************************************************************************/

#include "sensor_thread.h"
#include "Test/testbench.h"

#include "adc.h"
#include "control_thread.h"
#include "fault.h"
#include "gpio.h"
#include "trace.h"

#include <stdint.h>
#include <stdbool.h>

#define SENSOR_THREAD_STACK_SIZE				1024
#define SENSOR_THREAD_PREEMPTION_THRESHOLD		SENSOR_THREAD_PRIORITY
#define SENSOR_THREAD_NAME						"Sensor Thread"

#define THROTTLE_ADC_HANDLE_1		&hadc1	// PA3 (A0 on CN9)
#define THROTTLE_ADC_HANDLE_2		&hadc2	// PB1 (A3 on CN9)

#define THROTTLE_INPUT_MAX			((1 << THROTTLE_INPUT_RESOLUTION) - 1)
#define THROTTLE_SCALED_MAX			((1 << THROTTLE_SCALED_RESOLUTION) - 1)

#define SCALE_THROTTLE(T)			(T >> (THROTTLE_INPUT_RESOLUTION - THROTTLE_SCALED_RESOLUTION))
#define THROTTLE_DEADZONE			((UINT) (THROTTLE_DEADZONE_FRACTION * (float) THROTTLE_SCALED_MAX))
#define THROTTLE_MAX_DIFF			((UINT) (THROTTLE_MAX_DIFF_FRACTION * (float) THROTTLE_SCALED_MAX))

#define SENSOR_THREAD_TICK_RATE		100

/**
 * @brief Thread managing sensor measurements and inputs
 */
TX_THREAD sensor_thread;

/**
 * @brief Timer handling thread wakeup tick
 */
TX_TIMER sensor_thread_tick_timer;

/*
 * return codes
 */
#define ADC_OK	0x0000
#define ADC_ERR	0x0001

/*
 * function prototypes
 */
void sensor_thread_entry(ULONG thread_input);

#if (!RUN_THROTTLE_TESTBENCH)
UINT read_throttle();
UINT read_adc_blocking(ADC_HandleTypeDef* adc_handle, UINT* data_ptr);
#endif

/**
 * @brief 		Initialise sensor thread
 * 
 * @param[in]	stack_pool_ptr	Pointer to start of application stack area
 * 
 * @return		See ThreadX return codes
 */
UINT sensor_thread_init(TX_BYTE_POOL* stack_pool_ptr)
{
	VOID* thread_stack_ptr;

	UINT ret = tx_byte_allocate(stack_pool_ptr,
								&thread_stack_ptr,
								SENSOR_THREAD_STACK_SIZE,
								TX_NO_WAIT);

	if (ret == TX_SUCCESS)
	{
		ret  = tx_thread_create(&sensor_thread,
								SENSOR_THREAD_NAME,
								sensor_thread_entry, 
								0,
								thread_stack_ptr,
								SENSOR_THREAD_STACK_SIZE,
								SENSOR_THREAD_PRIORITY,
								SENSOR_THREAD_PREEMPTION_THRESHOLD,
								TX_NO_TIME_SLICE,
								TX_AUTO_START);
	}

	return ret;
}

/**
 * @brief 	Terminate sensor thread
 * 
 * @return 	See ThreadX return codes
 */
UINT sensor_thread_terminate()
{
	return tx_thread_terminate(&sensor_thread);
}

/**
 * @brief Sensor thread tick function
 */
static void sensor_thread_tick(ULONG input)
{
	tx_thread_resume(&sensor_thread);
}

/**
 * @brief Sensor thread entry function
 *
 * @param[in]	thread_input	Unused input
 */
void sensor_thread_entry(ULONG thread_input)
{
	// prevent compiler warnings as input is not used for the moment
	(VOID) thread_input;

	// create timer
	UINT ret = tx_timer_create(&sensor_thread_tick_timer, 
								"Sensor Thread Tick Timer", 
								sensor_thread_tick, 0, 
								TX_TIMER_TICKS_PER_SECOND / SENSOR_THREAD_TICK_RATE, 
								TX_TIMER_TICKS_PER_SECOND / SENSOR_THREAD_TICK_RATE, 
								TX_NO_ACTIVATE);

	if (ret != TX_SUCCESS)
	{
		return;
	}

	// loop forever
	while(1)
	{
		// check for fault state
#if (RUN_FAULT_STATE_TESTBENCH)
		testbench_fault_state();
#endif

		// read throttle
#if (RUN_THROTTLE_TESTBENCH)
		uint32_t throttle = testbench_throttle();
#else
		ULONG throttle = (ULONG) read_throttle();
#endif

		// transmit throttle to control thread
		ret = tx_queue_send(&throttle_input_queue, (ULONG*) &throttle, TX_NO_WAIT);

		if (ret == TX_QUEUE_FULL)
		{
			critical_fault(CRITICAL_FAULT_QUEUE_FULL);
		}

		// sleep thread to allow other threads to run
		// tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 10);
		tx_timer_activate(&sensor_thread_tick_timer);
		tx_thread_suspend(&sensor_thread);
	}
}

#if (!RUN_THROTTLE_TESTBENCH)
/**
 * @brief 	Read throttle
 *
 * @details	The ADC readings from the throttle pedal are scaled to the resolution specified by
 * 			THROTTLE_SCALED_RESOLUTION. If the maximum allowed discrepancy between the two ADC
 * 			readings is exceeded, the throttle is cut off. Below the set dead-zone, the throttle
 * 			is also cut off (this prevents an input very close to zero from producing a slight
 *			output).
 *
 * @return	Scaled throttle reading
 */
UINT read_throttle()
{
	// read from each input
	UINT reading_1 = 0;
	UINT reading_2 = 0;

	read_adc_blocking(THROTTLE_ADC_HANDLE_1, &reading_1);
	read_adc_blocking(THROTTLE_ADC_HANDLE_2, &reading_2);

	// scale readings
	reading_1 = SCALE_THROTTLE(reading_1);
	reading_2 = SCALE_THROTTLE(reading_2);

	// check for discrepancy between readings
#if THROTTLE_ENABLE_DIFF_CHECK

	UINT diff = (reading_2 > reading_1) ? (reading_2 - reading_1) : (reading_1 - reading_2);

	if (diff > THROTTLE_MAX_DIFF)
	{
		// maximum allowable discrepancy exceeded
		// TODO: error indication?
		return 0;
	}

#endif

	// calculate average of both readings
	UINT throttle = (reading_1 + reading_2) / 2;

	trace_log_event(TRACE_THROTTLE_INPUT_EVENT, (ULONG) throttle, 0, 0, 0);
	return throttle;
}

/**
 * @brief 		Blocking read from ADC
 *
 * @param[in]	adc_handle		ADC handle
 * @param[in]	data_ptr		Pointer to UINT to store 16 bit reading
 *
 * @retval		ADC_OK	 		ADC read successfully
 * @retval		ADC_ERR 		Error reading ADC
 */
UINT read_adc_blocking(ADC_HandleTypeDef* adc_handle, UINT* data_ptr)
{
	/*
	 * Read from ADC
	 * - 16 bit resolution, max 0xFFFF
	 * - 3v3 reference
	 */

	// start ADC
	// wait for conversion to complete
	if (HAL_ADC_Start(adc_handle) == HAL_OK)
	{
		// block until conversion complete
		HAL_ADC_PollForConversion(adc_handle, HAL_MAX_DELAY);

		// read conversion data
		*data_ptr = HAL_ADC_GetValue(adc_handle);

		/*
		 * alternatively to convert to float:
		 * *data_ptr = 3.3f * ((float) adc_raw / (float) 0xFFFF);
		 */
		return ADC_OK;
	}

	return ADC_ERR;
}
#endif
