/***************************************************************************
 * @file   messaging.c
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2021-11-30
 * @brief  Inter-thread messaging system implementation
 ***************************************************************************/

#include "messaging_system.h"
#include <stdint.h>

/**
 * @brief Queue for sensor messages
 */
TX_QUEUE torque_request_queue;
TX_QUEUE control_input_queue;

static uint8_t torque_request_queue_memory [TORQUE_REQUEST_QUEUE_SIZE];
static uint8_t control_input_queue_memory [CONTROL_INPUT_QUEUE_SIZE];

/**
 * @brief Initialise message passing system
 */
UINT message_system_init()
{
	UINT ret;

	// control -> CAN torque request queue
	ret = tx_queue_create(&torque_request_queue, TORQUE_REQUEST_QUEUE_NAME, TORQUE_REQUEST_QUEUE_MESSAGE_SIZE,
			(VOID*) torque_request_queue_memory, TORQUE_REQUEST_QUEUE_SIZE);

	// sensor -> control input queue
	if (ret == TX_SUCCESS)
	{
		ret = tx_queue_create(&control_input_queue, CONTROL_INPUT_QUEUE_NAME, CONTROL_INPUT_QUEUE_MESSAGE_SIZE,
				(VOID*) control_input_queue_memory, CONTROL_INPUT_QUEUE_SIZE);
	}

	return ret;
}

/**
 * @brief 		Post a message to a queue (blocking)
 *
 * @note		This is the bare minimum outline code. Need to add checks for message size etc.
 *
 * @param[in]	message_ptr		Pointer to message memory
 * @param[in]	queue_ptr		Pointer to TX_QUEUE to send to
 *
 * @return		See TX_QUEUE API
 */
UINT message_post(VOID* message_ptr, TX_QUEUE* queue_ptr)
{
	return tx_queue_send(queue_ptr, message_ptr, TX_WAIT_FOREVER);
}

/**
 * @brief 		Receive a message from a queue (blocking)
 *
 * @param[in]	dest_ptr		Destination to store message from queue
 * @param[in]	queue_ptr		Pointer to TX_QUEUE to receive from
 *
 * @return		See TX_QUEUE API
 */
UINT message_receive(VOID* dest_ptr, TX_QUEUE* queue_ptr)
{
	return tx_queue_receive(queue_ptr, dest_ptr, TX_WAIT_FOREVER);
}



