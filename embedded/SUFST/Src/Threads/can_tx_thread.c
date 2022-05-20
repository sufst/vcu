/***************************************************************************
 * @file   can_tx_thread.c
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2021-11-30
 * @brief  CAN transmit thread implementation
 ***************************************************************************/

#include "can_tx_thread.h"
#include "config.h"
#include "tx_api.h"

#include "messaging_system.h"
#include "pm100.h"
#include "trace.h"

/**
 * @brief Thread for CAN transmit
 */
TX_THREAD can_tx_thread;

/**
 * @brief Thread entry function for CAN transmit thread
 *
 * @param[in]	thread_input	(Unused) thread input
 */
void can_tx_thread_entry(ULONG thread_input)
{
	(void) thread_input;

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

#if INVERTER_SPEED_MODE
		// map torque to speed request and send to inverter through CAN
		// UINT speed = foo(message.value);
		UINT speed_request = message.value / 100;
		pm100_speed_request(speed_request);
#else
	#if !(INVERTER_DISABLE_TORQUE_REQUESTS)
		// send the torque request to inverter through CAN
		UINT torque_request = message.value;
		pm100_status_t status = pm100_torque_request(torque_request);

		// debug logging
		if (status == PM100_OK)
		{
			trace_log_event(TRACE_TORQUE_REQUEST_EVENT, (ULONG) torque_request, message.timestamp, 0, 0);
		}
		else 
		{
			trace_log_event(TRACE_INVERTER_ERROR, (ULONG) status, message.timestamp, 0, 0);
		}
	#endif
#endif


	}
}
