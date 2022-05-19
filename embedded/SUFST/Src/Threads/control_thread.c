/***************************************************************************
 * @file   control_thread.c
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2021-11-30
 * @brief  Control thread implementation
 ***************************************************************************/

#include "control_thread.h"
#include "tx_api.h"

#include "driver_profiles.h"
#include "messaging_system.h"
#include "fault_state_thread.h"

/**
 * @brief Thread for control task
 */
TX_THREAD control_thread;

/*
 * function prototypes
 */
UINT map_input(UINT input);

/*
 * @brief Thread entry function for control_thread
 *
 * @param[in]	thread_input	(Unused) thread input
 */
void control_thread_entry(ULONG thread_input)
{
	// not using input, prevent compiler warning
	(VOID) thread_input;

	// look-up the driver profile
	const driver_profile_t* driver_profile_ptr;
	UINT ret = driver_profile_lookup(&driver_profile_ptr, SELECTED_DRIVER_PROFILE);

	if (ret != DRIVER_PROFILE_FOUND)
	{
		enter_fault_state();
	}

	// loop forever
	while (1)
	{
		// wait for a message to enter the control input queue
		// -> thread suspended until message received
		control_input_message_t input_message;
		UINT ret = message_receive(&input_message, &control_input_queue);

		if (ret != TX_SUCCESS) 
		{
			// should never happen, drop message if it does
			continue;
		}

		// apply the thottle curve
		UINT torque_request = apply_torque_map(driver_profile_ptr, input_message.input);

		// create and send the torque request to the CAN thread
		torque_request_message_t torque_message;
		torque_message.value = torque_request;
		torque_message.timestamp = input_message.timestamp;

		message_post((VOID*) &torque_message, &torque_request_queue);
	}
}