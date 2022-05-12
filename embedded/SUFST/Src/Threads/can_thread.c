/***************************************************************************
 * @file   can_thread.c
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2021-11-30
 * @brief  Control thread implementation
 ***************************************************************************/

#include "can_thread.h"
#include "tx_api.h"

#include "can_driver.h"
#include "fdcan.h"
#include "messaging_system.h"
#include "pm100.h"

/**
 * @brief Thread for CAN transmit
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
		// wait for message to enter torque request queue
		// -> thread suspended until message arrives
		torque_request_message_t message;
		UINT ret = message_receive((VOID*) &message, &torque_request_queue);

		if (ret != TX_SUCCESS) 
		{
			// should never happen but handle it for safety
			continue;
		}

#if RUN_SPEED_MODE
		// map torque to speed request and send to inverter through CAN
		// UINT speed = foo(message.value);
		pm100_speed_command_tx(message.value);
#else
		// send the torque request to inverter through CAN
		pm100_torque_command_tx(message.value);
#endif

	}
}