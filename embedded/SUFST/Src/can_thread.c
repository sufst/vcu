/***************************************************************************
 * @file   can_thread.c
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2021-11-30
 * @brief  Control thread implementation
 ***************************************************************************/

#include "can_thread.h"
#include "gpio.h"

/**
 * @brief Thread for control task
 */
TX_THREAD can_thread;

/*
 * @brief Thread entry function for control_thread
 *
 * @param[in]	thread_input	(Unused) thread input
 */
void can_thread_entry(ULONG thread_input)
{
	// not using input, prevent compiler warning
	(VOID) thread_input;

	// loop forever
	while (1)
	{
		// ... do stuff
		HAL_GPIO_TogglePin(GPIOB, LED_GREEN_Pin);

		// sleep this thread to allow other threads to run
		tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
	}
}
