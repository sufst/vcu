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
#include <stdint.h>

/**
 * @brief Thread managing the sensor measurements
 */
TX_THREAD sensor_thread;
uint32_t adc_raw[10];


/*
 * return codes
 */
#define ADC_OK	0x0000
#define ADC_ERR	0x0001


/*
 * function prototypes
 */
UINT test_adc_blocking();

/**
 * @brief Sensor thread entry function
 *
 * @param[in]	thread_input	Unused input
 */
void sensor_thread_entry(ULONG thread_input)
{
	// prevent compiler warnings as input is not used for the moment
	(VOID) thread_input;

	// ADC reading test
	float voltage;

	if (test_adc_blocking(&voltage) == ADC_OK)
	{
 		__asm__("NOP"); // breakpoint
	}

	// loop forever
	while(1)
	{
		// sleep thread to allow other threads to run
		tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
	}

}

/**
 * @brief 		Demo code for blocking read from ADC
 *
 * @param[in]	voltage_ptr		Pointer to float to store voltage reading
 *
 * @return		ADC_OK if ADC read successfully
 * 				ADC_ERR otherwise
 */
UINT test_adc_blocking(float* voltage_ptr)
{
	/*
	 * Read from ADC
	 * - 16 bit resolution, max 0xFFFF
	 * - 3v3 reference
	 * - ADC1 IN3 single ended reads from pin D12 (PA6)
	 * - connect to 1.8v on JP5 as a simple test
	 */

	// start ADC
	// wait for conversion to complete
	if (HAL_ADC_Start(&hadc1) == HAL_OK)
	{
		// block until conversion complete
		HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);

		// read conversion data
		uint32_t adc_raw = HAL_ADC_GetValue(&hadc1);
		*voltage_ptr = 3.3f * ((float) adc_raw / (float) 0xFFFF);

		return ADC_OK;
	}

	return ADC_ERR;
}

