/***************************************************************************
 * @file   control_thread.c
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2021-11-30
 * @brief  Control thread implementation
 ***************************************************************************/

#include "control_thread.h"
#include "config.h"
#include "tx_api.h"

#include "driver_profiles.h"
#include "fault.h"
#include "messaging_system.h"

#define CONTROL_THREAD_STACK_SIZE			1024
#define CONTROL_THREAD_PREEMPTION_THRESHOLD CONTROL_THREAD_PRIORITY
#define CONTROL_THREAD_NAME					"Control Thread"

/**
 * @brief Thread for control task
 */
TX_THREAD control_thread;

/*
 * function prototypes
 */
void control_thread_entry(ULONG thread_input);

/**
 * @brief 		Initialise control thread
 * 
 * @param[in]	stack_pool_ptr 	Pointer to start of application stack area
 * 
 * @return 		See ThreadX return codes
 */
UINT control_thread_init(TX_BYTE_POOL* stack_pool_ptr)
{
	VOID* thread_stack_ptr;

	UINT ret = tx_byte_allocate(stack_pool_ptr, 
								&thread_stack_ptr,
								CONTROL_THREAD_STACK_SIZE,
								TX_NO_WAIT);

	if (ret == TX_SUCCESS)
	{
		ret = tx_thread_create(&control_thread,
								CONTROL_THREAD_NAME,
								control_thread_entry,
								0,
								thread_stack_ptr,
								CONTROL_THREAD_STACK_SIZE,
								CONTROL_THREAD_PRIORITY,
								CONTROL_THREAD_PREEMPTION_THRESHOLD,
								TX_NO_TIME_SLICE,
								TX_AUTO_START);
	}

	return ret;
}

/**
 * @brief 	Terminate control thread
 * 
 * @return 	See ThreadX return codes
 */
UINT control_thread_terminate()
{
	return tx_thread_terminate(&control_thread);
}

/**
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

	if (driver_profile_lookup(&driver_profile_ptr, SELECTED_DRIVER_PROFILE) != DRIVER_PROFILE_FOUND)
	{
		critical_fault(CRITICAL_FAULT_DRIVER_PROFILE_NOT_FOUND);
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