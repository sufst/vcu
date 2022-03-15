/***************************************************************************
 * @file   fault_state_thread.c
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2022-03-15
 * @brief  Fault state thread
 ***************************************************************************/

#include "fault_state_thread.h"
#include "config.h"

#include "can_thread.h"
#include "control_thread.h"
#include "sensor_thread.h"
#include "gpio.h"

/**
 * @brief Thread for fault state
 */
TX_THREAD fault_state_thread;

/**
 * @brief 		Fault state thread entry function
 *
 * @param[in]	thread_input	Unused input
 */
void fault_state_thread_entry(ULONG thread_input)
{
	// TODO: time-critical actions

	// shut down other threads
	// note: this thread has the highest priority and will not be pre-empted
	tx_thread_terminate(&sensor_thread);
	tx_thread_terminate(&control_thread);
	tx_thread_terminate(&can_thread);

	// loop forever, do not leave fault state
	// flash the LED
	while (1)
	{
		HAL_GPIO_TogglePin(RED_LED_GPIO_Port, RED_LED_Pin);
		tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / FAULT_STATE_LED_BLINK_RATE);
	}

}
