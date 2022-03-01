/***************************************************************************
 * @file   messaging.c
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2021-11-30
 * @brief  Inter-thread messaging system implementation
 ***************************************************************************/

#include "messaging_system.h"
#include <stdint.h>

#define TIMESTAMP_TO_MS		(1000 / TX_TIMER_TICKS_PER_SECOND)
#define MESSAGE_POOL_NAME 	"message-pool"
#define MESSAGE_POOL_SIZE 	100

#define MESSAGE_PRIORITY_HIGH	0
#define MESSAGE_PRIORITY_LOW	1

/**
 * @brief Queue for sensor messages
 */
TX_QUEUE torque_request_queue;
TX_QUEUE control_input_queue;

static uint8_t torque_request_queue_memory [TORQUE_REQUEST_QUEUE_SIZE];
static uint8_t control_input_queue_memory [CONTROL_INPUT_QUEUE_SIZE];
static uint8_t msg_pool [MESSAGE_POOL_SIZE];

static queue_item_t priorityQueue [MESSAGE_POOL_SIZE];

typedef enum {
  TORQUE_MESSAGE,
  CONTROL_MESSAGE,
} msg_type;

typedef struct {
  UINT priority;
  VOID* msg_ptr;
  msg_type type;
  UINT timestamp;
} queue_item_t;

typedef struct {
	UINT value;
	UINT timestamp;
} torque_msg_t;

typedef struct {
	UINT input;
	UINT timestamp;
} control_msg_t;

/**
 * @brief Allocate space in the message pool for a message
 *
 * @param[out]  msg_dest_ptr    Pointer to store location of allocated message space
 * @param[in]   msg_size        Size of message in bytes
 *
 * @return Status code from tx_byte_allocate
 */
UINT message_alloc(VOID** msg_dest_ptr, UINT msg_size) {
	return tx_byte_allocate(msg_pool, msg_dest_ptr, msg_size, TX_WAIT_FOREVER);
}

/**
 * @brief Send a prioritised message through a particular queue
 *
 * @param[in]	queue_ptr		Pointer to queue
 * @param[in]	message_ptr		Pointer to message in message pool
 * @param[in]	message_type	Enumerated message type
 * @param[in]	priority		Message priority
 */
UINT message_send(TX_QUEUE* queue_ptr, VOID* msg_ptr, msg_type type, UINT priority){
	queue_item_t* item;
	item->priority = priority;
	item->msg_ptr = msg_ptr;
	item->type = type;
	message_set_timestamp(&msg_ptr->timestamp);
	return tx_queue_send(queue_ptr, (VOID*) &item, TX_WAIT_FOREVER);
}

/**
 * @brief Block thread until items enter the queue, then buffer items by priority
 *
 * @param[in]	queue_ptr	Pointer to queue to block on
 */
UINT message_read_queue(TX_QUEUE* queue_ptr){
	queue_item_t* item;
	UINT ret = tx_queue_receive(queue_ptr, (VOID*) &item, TX_WAIT_FOREVER);
	if (ret == TX_SUCCESS) {

	}
};

/**
 * @brief Get the highest priority message with the earliest timestamp
 *
 * @param[in]	queue_ptr			Pointer to queue associated with message
 * @param[in]	message_dest_ptr	Destination to store retrieved message
 * @param[in]	type_ptr			Pointer to destination to store retreived message type
 */
UINT message_get(TX_QUEUE* queue_ptr, VOID** message_dest_ptr, msg_type* type_ptr){

};

/**
 * @brief Frees the allocated memory associated with a message
 *
 * @param[in]	msg_ptr		Pointer to message
 *
 * @return Status code from tx_byte_release
 */
UINT message_delete(VOID* msg_ptr) {
	return tx_byte_release(msg_ptr);
}


VOID sending_thread()
{
	// place to store the message address
	torque_msg_t* msg_ptr;

	// allocate space in the pool
	UINT ret = message_alloc((VOID**) &msg_ptr, sizeof(torque_msg_t));

	// write data into the allocated space
	message_set_timestamp(&msg_ptr->timestamp);
	msg_ptr->value = 10;

	// send the message with a certain priority
	message_send(&torque_request_queue, (VOID*) msg_ptr, TORQUE_MESSAGE, MESSAGE_PRIORITY_HIGH);

}

//VOID receiving_threads()
//{
//	// update message system
//	message_read_queue(&torque_request_queue);
//
//	// get the next message
//	VOID* message_ptr;
//	msg_type type;
//
//	while (message_get(&torque_request_queue, (VOID**) &message_ptr, &type))
//	{
//		switch (type)
//		{
//		case TORQUE_MESSAGE:
//		{
//			torque_msg_t* msg_ptr = (torque_msg_t*) msg_ptr;
//
//			// do stuff with the message...
//
//			break;
//		}
//
//		default:
//			break;
//		}
//
//	}
//}

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
	if (ret == TX_SUCCESS) {
		ret = tx_queue_create(&control_input_queue, CONTROL_INPUT_QUEUE_NAME, CONTROL_INPUT_QUEUE_MESSAGE_SIZE,
				(VOID*) control_input_queue_memory, CONTROL_INPUT_QUEUE_SIZE);
	}

	if (ret == TX_SUCCESS) {
		ret = tx_byte_pool_create(msg_pool, MESSAGE_POOL_NAME, msg_pool, MESSAGE_POOL_SIZE);
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
UINT message_post(TX_QUEUE* queue_ptr, VOID* msg_ptr, ULONG msg_size)
{
	return tx_queue_send(queue_ptr, msg_ptr, TX_WAIT_FOREVER);
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

/**
 * @brief 		Sets the timestamp of the message
 *
 * @details		At the moment this uses the time since processor startup in milliseconds as
 * 				the timestamp. In future this might need to be synchronised with other parts
 * 				of the larger system.
 *
 * @note		The timestamp will overflow after around 49.7 days.
 *
 * @param[in]	timestamp_ptr	UINT pointer to message struct field to store timestamp in
 */
void message_set_timestamp(UINT* timestamp_ptr)
{
	ULONG current_time = tx_time_get() * TIMESTAMP_TO_MS;
	*timestamp_ptr = (UINT) current_time;	// this cast is safe on this ThreadX port as sizeof(UINT) is the same as sizeof(ULONG)
}




