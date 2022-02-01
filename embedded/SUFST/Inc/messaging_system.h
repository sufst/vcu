/***************************************************************************
 * @file   messaging_system.h
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2021-11-30
 * @brief  Inter-thread messaging system function prototypes and definitions
 ***************************************************************************/

#ifndef MESSAGING_SYSTEM_H
#define MESSAGING_SYSTEM_H

#include "tx_api.h"
#include "message_types.h"

/*
 * queue definitions
 */
#define TORQUE_REQUEST_QUEUE_MESSAGE_SIZE	2
#define TORQUE_REQUEST_QUEUE_SIZE			100
#define TORQUE_REQUEST_QUEUE_NAME			"Torque Request Queue"

/*
 * queues for the messaging system
 */
extern TX_QUEUE torque_request_queue;

/*
 * return codes
 */
#define MESSAGE_POSTED			0x00
#define MESSAGE_SIZE_INVALID	0x01
#define MESSAGE_QUEUE_FULL		0x02
#define MESSAGE_QUEUE_INVALID	0x03

/*
 * function prototypes
 */
UINT message_system_init();
UINT message_post(void* message_ptr, TX_QUEUE* queue_ptr);
UINT message_receive(void* dest_ptr, TX_QUEUE* queue_ptr);

#endif
