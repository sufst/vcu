/***************************************************************************
 * @file   sensor_thread.c
 * @author Cosmin-Andrei Tamas (cat1g19@soton.ac.uk)
 * @date   2021-11-30
 * @brief  Sensor thread
 ***************************************************************************/

#include "sensor_thread.h"
#include "gpio.h"
#include "adc.h"
#include <stdint.h>

/**
 * @brief Thread managing the sensor measurements
 */
TX_THREAD sensor_thread;
uint32_t adc_raw[10];


/*
 * function prototypes
 */
void test_adc_blocking();

/**
 * @brief Sensor thread entry function
 *
 * @param[in]	thread_input	Unused input
 */
void sensor_thread_entry(ULONG thread_input)
{
	// prevent compiler warnings as input is not used for the moment
	(VOID) thread_input;

	test_adc_blocking();

	// loop forever
	while(1)
	{
		// sleep thread to allow other threads to run
		tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
	}

}

/**
 * @brief Demo code for blocking read from ADC
 */
void test_adc_blocking()
{
	/*
	 * Read from ADC
	 * - 16 bit resolution, max 0xFFFF
	 * - 3v3 reference
	 * - ADC1 IN3 single ended reads from pin D12 (PA6)
	 * - connect to 1.8v on JP5 as a simple test
	 */

	// start ADC
	HAL_StatusTypeDef ret = HAL_ADC_Start(&hadc1);

	// wait for conversion to complete
	if (ret == HAL_OK)
	{
		// block until conversion complete
		HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);

		// read conversion data
		uint32_t adc_raw = HAL_ADC_GetValue(&hadc1);
		float voltage = 3.3f * ((float) adc_raw / (float) 0xFFFF);
		(void) voltage;
	}
}

