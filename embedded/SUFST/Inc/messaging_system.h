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

#define CONTROL_INPUT_QUEUE_MESSAGE_SIZE	2
#define CONTROL_INPUT_QUEUE_SIZE			100
#define CONTROL_INPUT_QUEUE_NAME			"Control Input Queue"

/*
 * queues for the messaging system
 */
extern TX_QUEUE torque_request_queue;
extern TX_QUEUE control_input_queue;

/*
 * function prototypes
 */
UINT message_system_init();
UINT message_post(VOID* message_ptr, TX_QUEUE* queue_ptr);
UINT message_receive(VOID* dest_ptr, TX_QUEUE* queue_ptr);
void message_set_timestamp(UINT* timestamp_ptr);

#endif
