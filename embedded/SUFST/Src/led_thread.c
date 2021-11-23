/***************************************************************************
 * @file   led_thread.c
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2021-11-09
 * @brief  LED thread
 ***************************************************************************/

#include "led_thread.h"

/**
 * @brief Test thread to flash an LED
 */
TX_THREAD led_thread;

/*
 * @brief Thread entry function for led_thread
 *
 * @param[in]	thread_input	(Unused) thread input
 */
void led_thread_entry(ULONG thread_input)
{
	// not using input, prevent compiler warning
	(VOID) thread_input;

	// loop forever
	while (1)
	{
		// ... do stuff

		// sleep this thread to allow other threads to run
		tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
	}
}
