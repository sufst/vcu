/***************************************************************************
 * @file   sensor_thread.c
 * @author Cosmin-Andrei Tamas (cat1g19@soton.ac.uk)
 * @date   2021-11-30
 * @brief  Sensor thread
 ***************************************************************************/

#include "sensor_thread.h"
#include "gpio.h"

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
		// flash the red led
		HAL_GPIO_TogglePin(RED_LED_GPIO_Port, RED_LED_Pin);

		// sleep thread to allow other threads to run
		tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
	}
}
