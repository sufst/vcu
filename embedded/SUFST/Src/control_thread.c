/***************************************************************************
 * @file   control_thread.c
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2021-11-30
 * @brief  Control thread implementation
 ***************************************************************************/

#include "control_thread.h"
#include "messaging_system.h"
#include "tx_api.h"
#include "message_types.h"

/**
 * @brief Thread for control task
 */
TX_THREAD control_thread;

/*
 * @brief Thread entry function for control_thread
 *
 * @param[in]	thread_input	(Unused) thread input
 */
void control_thread_entry(ULONG thread_input)
{
	// not using input, prevent compiler warning
	(VOID) thread_input;

	UINT ret;

	torque_request_message_t message;

	message.priority = 1;
	message.value = 6;

	ret = message_post((void*) &message, &torque_request_queue);
	(void) ret;

	// loop forever
	while (1)
	{
		// ... do stuff

		// sleep this thread to allow other threads to run
		tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
	}
}
