/***************************************************************************
 * @file   sensor_thread.c
 * @author Cosmin-Andrei Tamas (cat1g19@soton.ac.uk)
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2021-11-30
 * @brief  Sensor thread
 ***************************************************************************/

#include "sensor_thread.h"
#include "gpio.h"
#include "adc.h"
#include "messaging_system.h"

#include <stdint.h>
#include <stdbool.h>

/*
 * configuration
 */
#define THROTTLE_INPUT_RESOLUTION	16		// resolution of ADC
#define THROTTLE_SCALED_RESOLUTION	10		// scaled resolution sent to control thread
#define THROTTLE_DEADZONE_FRACTION	0.02	// fractional dead-zone below which throttle always zero
#define THROTTLE_MAX_DIFF_FRACTION	0.05	// fractional maximum allowed difference between ADC readings
#define THROTTLE_ENABLE_DIFF_CHECK	1		// set to 1 to enable check for discrepancy between ADC readings
#define THROTTLE_ENABLE_DEADZONE	1		// set to 1 to enable dead-zone

/*
 * macros
 */
#define THROTTLE_ADC_HANDLE_1	&hadc1	// PA3 (A0 on CN9)
#define THROTTLE_ADC_HANDLE_2	&hadc2	// PB1 (A3 on CN9)

#define THROTTLE_INPUT_MAX			((1 << THROTTLE_INPUT_RESOLUTION) - 1)
#define THROTTLE_SCALED_MAX			((1 << THROTTLE_SCALED_RESOLUTION) - 1)

#define SCALE_THROTTLE(T)			(T >> (THROTTLE_INPUT_RESOLUTION - THROTTLE_SCALED_RESOLUTION))
#define THROTTLE_DEADZONE			((UINT) (THROTTLE_DEADZONE_FRACTION * (float) THROTTLE_SCALED_MAX))
#define THROTTLE_MAX_DIFF			((UINT) (THROTTLE_MAX_DIFF_FRACTION * (float) THROTTLE_SCALED_MAX))

/**
 * @brief Thread managing sensor measurements and inputs
 */
TX_THREAD sensor_thread;

/*
 * return codes
 */
#define ADC_OK	0x0000
#define ADC_ERR	0x0001

/*
 * function prototypes
 */
UINT read_throttle();
UINT read_adc_blocking(ADC_HandleTypeDef* adc_handle, UINT* data_ptr);

/**
 * @brief Sensor thread entry function
 *
 * @param[in]	thread_input	Unused input
 */
void sensor_thread_entry(ULONG thread_input)
{
	// prevent compiler warnings as input is not used for the moment
	(VOID) thread_input;

	// loop forever
	while(1)
	{
		// read throttle and send it to control thread
		control_input_message_t message;
    message.input = read_throttle();
    
    message_set_timestamp(&message.timestamp);
		message_post((VOID*) &message, &control_input_queue);

		// sleep thread to allow other threads to run
		// TODO: this is temporary, need to decide on wake up rate
		tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);

	}

}

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
	// note: right shift 1 is a faster equivalent of divide by 2
	UINT average = (reading_1 + reading_2) >> 1;

	// apply dead-zone
#if THROTTLE_ENABLE_DEADZONE

	if (average < THROTTLE_DEADZONE)
	{
		return 0;
	}

#endif

	// input legal and throttle outside dead-zone
	return average;
}

/**
 * @brief 		Demo code for blocking read from ADC
 *
 * @param[in]	adc_handle		ADC handle
 * @param[in]	data_ptr		Pointer to UINT to store 16 bit reading
 *
 * @return		ADC_OK if ADC read successfully
 * 				ADC_ERR otherwise
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

