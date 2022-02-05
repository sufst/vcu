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
		control_input_message_t message;
		UINT ret = message_receive(&message, &control_input_queue);

		// TODO: pass input to *actual* control algorithm and produce torque request
		// 		(using placeholder function at the moment)
		UINT torque_request = pid_control(message.input);

		// TODO: send the torque request to the can thread
		(void) torque_request;

	}
}

/**
 * @brief Placeholder for PID algorithm
 */
UINT pid_control(UINT input)
{
	return input;
}
