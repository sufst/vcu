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
#define SENSOR_THREAD_TICK_RATE					100 // times per second

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

#if !RUN_THROTTLE_TESTBENCH
uint32_t read_throttle();
void scale_throttle_adc_reading(uint32_t* adc_reading_ptr);
#endif

/**
 * @brief 		Initialise sensor thread
 * 
 * @param[in]	stack_pool_ptr	Pointer to start of application stack area
 * 
 * @return		See ThreadX return codes
 */
UINT sensor_thread_create(TX_BYTE_POOL* stack_pool_ptr)
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
								TX_DONT_START);
	}

	return ret;
}

/**
 * @brief	Starts the sensor thread
 * 
 * @return 	See ThreadX return codes
 */
UINT sensor_thread_start()
{
	return tx_thread_resume(&sensor_thread);
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
#if RUN_FAULT_STATE_TESTBENCH
		testbench_fault_state();
#endif

		// read throttle
#if RUN_THROTTLE_TESTBENCH
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

#if !RUN_THROTTLE_TESTBENCH
/**
 * @brief 	Read throttle
 *
 * @details	The ADC readings from the throttle pedal are scaled to the resolution specified by
 * 			THROTTLE_SCALED_RESOLUTION. If the maximum allowed discrepancy between the two ADC
 * 			readings is exceeded, the throttle is cut off and a critical error is raised.
 * 
 * @note 	Throttle discrepancy checking currently does not scale to more than 2 ADCs
 *
 * @return	Scaled throttle reading
 */
uint32_t read_throttle()
{
	// start ADC readings in parallel
	ADC_HandleTypeDef* adc_handles[] = {
		&hadc1,	// PA3 (A0 on CN9)
		&hadc2,	// PB1 (A3 on CN9)
	};

	const uint32_t num_adcs = sizeof(adc_handles) / sizeof(adc_handles[0]);

	for (uint32_t i = 0; i < num_adcs; i++)
	{
		if (HAL_ADC_Start(adc_handles[i]) != HAL_OK)
		{
			critical_fault(CRITICAL_FAULT_HAL);
		}
	}

	// fetch ADC results (blocking)
	uint32_t adc_readings[num_adcs];
	uint32_t accumulator = 0;

	for (uint32_t i = 0; i < num_adcs; i++)
	{
		if (HAL_ADC_PollForConversion(adc_handles[i], HAL_MAX_DELAY) == HAL_OK)
		{
			adc_readings[i] = HAL_ADC_GetValue(adc_handles[i]);
			scale_throttle_adc_reading(&adc_readings[i]);
			accumulator += adc_readings[i];
		}
		else  
		{
			critical_fault(CRITICAL_FAULT_HAL);
		}
	}

	// check for discrepancy between readings
#if THROTTLE_ENABLE_DIFF_CHECK

	uint32_t diff;
	
	if (adc_readings[1] > adc_readings[0])
	{
		diff = adc_readings[1] - adc_readings[0];
	}
	else
	{
		diff = adc_readings[0] - adc_readings[1];
	}

	if (diff > THROTTLE_MAX_DIFF)
	{
		critical_fault(CRITICAL_FAULT_THROTTLE_INPUT_DISCREPANCY);
		return 0;
	}

#endif

	// calculate average of both readings
	uint32_t throttle = accumulator / num_adcs;

	trace_log_event(TRACE_THROTTLE_INPUT_EVENT, (ULONG) throttle, 0, 0, 0);
	return throttle;
}

/**
 * @brief 			Scale raw ADC reading
 * 
 * @param[inout]	adc_reading_ptr		Pointer to ADC reading
 */
void scale_throttle_adc_reading(uint32_t* adc_reading_ptr)
{
	const uint32_t input_resolution = THROTTLE_INPUT_RESOLUTION;
	const uint32_t truncated_resolution = THROTTLE_SCALED_RESOLUTION;
	
	*adc_reading_ptr = *adc_reading_ptr >> (input_resolution - truncated_resolution);
}
#endif