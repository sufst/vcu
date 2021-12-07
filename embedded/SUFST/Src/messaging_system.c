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
static uint8_t torque_request_queue_memory [TORQUE_REQUEST_QUEUE_SIZE];

/**
 * @brief Initialise message passing system
 */
UINT message_system_init()
{
	UINT ret;

	ret = tx_queue_create(&torque_request_queue, TORQUE_REQUEST_QUEUE_NAME, TORQUE_REQUEST_QUEUE_MESSAGE_SIZE,
			(VOID*) torque_request_queue_memory, TORQUE_REQUEST_QUEUE_SIZE);

	return ret;
}
