/***************************************************************************
 * @file   can_thread.c
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2021-11-30
 * @brief  Control thread implementation
 ***************************************************************************/

#include "can_thread.h"
#include "tx_api.h"
#include "gpio.h"
#include "fdcan.h"
#include "messaging_system.h"

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

	UINT ret;

	torque_request_message_t received;

	ret = message_receive((void*) &received, &torque_request_queue);

	if (ret == TX_SUCCESS)
	{
		__asm__("NOP"); // breakpoint
	}

	// loop forever
	while (1)
	{
		// ... do stuff
		HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);

		// sleep this thread to allow other threads to run
		tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
	}
}
