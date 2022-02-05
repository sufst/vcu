/***************************************************************************
 * @file   control_thread.c
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2021-11-30
 * @brief  Control thread implementation
 ***************************************************************************/

#include "control_thread.h"
#include "messaging_system.h"
#include "tx_api.h"

/**
 * @brief Thread for control task
 */
TX_THREAD control_thread;

/*
 * function prototypes
 */
UINT pid_control(UINT input);

/*
 * @brief Thread entry function for control_thread
 *
 * @param[in]	thread_input	(Unused) thread input
 */
void control_thread_entry(ULONG thread_input)
{
	// not using input, prevent compiler warning
	(VOID) thread_input;

	// loop forever
	while (1)
	{
		// wait for a message to enter the control input queue
		// -> thread suspended until message received
		control_input_message_t input_message;
		UINT ret = message_receive(&input_message, &control_input_queue);

		if (ret != TX_SUCCESS) continue; // should never happen, drop message if it does

		// TODO: pass input to *actual* control algorithm and produce torque request
		// 		(using placeholder function at the moment)
		UINT torque_request = pid_control(input_message.input);

		// send the torque request to the can thread
		torque_request_message_t torque_message;
		torque_message.value = torque_request;

		message_post((VOID*) &torque_message, &torque_request_queue);

	}
}

/**
 * @brief Placeholder for PID algorithm
 */
UINT pid_control(UINT input)
{
	return input;
}
