/***************************************************************************
 * @file   messaging.c
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2021-11-30
 * @brief  Inter-thread messaging system implementation
 ***************************************************************************/

#include "messaging.h"
#include "stdint.h"

/**
 * @brief Queue for sensor messages
 */
TX_QUEUE sensor_message_queue;
static uint8_t sensor_queue_memory [SENSOR_QUEUE_SIZE];

/**
 * @brief Initialise message passing system
 */
UINT message_system_init()
{
	UINT ret;

	ret = tx_queue_create(&sensor_message_queue, SENSOR_QUEUE_NAME, SENSOR_QUEUE_MESSAGE_SIZE,
			(VOID*) sensor_queue_memory, SENSOR_QUEUE_SIZE);

	return ret;
}
