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

void sensor_thread_entry(ULONG thread_input)
{
	// prevent compiler warnings as input is not used for the moment
	(VOID) thread_input;

	while(1)
	{
		/*
		 * Read from ADC
		 * - 16 bit resolution, max 0xFFFF
		 * - 3v3 reference
		 * - reads from pin D12 (PA6)
		 * - connect to 1.8v on JP5 as a simple test
		 */
		HAL_ADC_Start(&hadc1);
		HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
		uint32_t adc_raw = HAL_ADC_GetValue(&hadc1);

		float voltage = 3.3f * ((float) adc_raw / (float) 0xFFFF);
		(void) voltage;

		// sleep thread to allow other threads to run
		tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
	}
}
