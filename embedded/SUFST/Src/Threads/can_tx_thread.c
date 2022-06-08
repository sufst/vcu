/***************************************************************************
 * @file   can_tx_thread.c
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2021-11-30
 * @brief  CAN transmit thread implementation
 ***************************************************************************/

#include "can_tx_thread.h"
#include "config.h"
#include "tx_api.h"

#include "pm100.h"
#include "trace.h"

#define CAN_TX_THREAD_STACK_SIZE			1024
#define CAN_TX_THREAD_PREEMPTION_THRESHOLD	CAN_TX_THREAD_PRIORITY
#define CAN_TX_THREAD_NAME					"CAN Tx Thread"

#define TORQUE_REQUEST_QUEUE_NAME			"CAN Tx Torque Request Queue"
#define TORQUE_REQUEST_QUEUE_ITEM_SIZE		TX_1_ULONG
#define TORQUE_REQUEST_QUEUE_SIZE			2

/**
 * @brief Thread for CAN transmit
 */
TX_THREAD can_tx_thread;

/**
 * @brief Torque request transmit queue
 */
TX_QUEUE torque_request_queue;

/**
 * @brief Torque request transmit queue memory area
 */
static ULONG torque_request_queue_mem[TORQUE_REQUEST_QUEUE_SIZE * TORQUE_REQUEST_QUEUE_ITEM_SIZE];

/*
 * function prototypes
 */ 
void can_tx_thread_entry(ULONG thread_input);

/**
 * @brief 		Create CAN transmit thread
 * 
 * @param[in]	stack_pool_ptr 	Pointer to start of application stack area
 * 
 * @return 		See ThreadX return codes
 */
UINT can_tx_thread_create(TX_BYTE_POOL* stack_pool_ptr)
{
	// create thread
	VOID* thread_stack_ptr;

	UINT ret = tx_byte_allocate(stack_pool_ptr, 
								&thread_stack_ptr,
								CAN_TX_THREAD_STACK_SIZE,
								TX_NO_WAIT);

	if (ret == TX_SUCCESS)
	{
		ret = tx_thread_create(&can_tx_thread,
								CAN_TX_THREAD_NAME,
								can_tx_thread_entry,
								0,
								thread_stack_ptr,
								CAN_TX_THREAD_STACK_SIZE,
								CAN_TX_THREAD_PRIORITY,
								CAN_TX_THREAD_PREEMPTION_THRESHOLD,
								TX_NO_TIME_SLICE,
								TX_DONT_START);
	}

	// create torque request queue
	if (ret == TX_SUCCESS)
	{
		ret = tx_queue_create(&torque_request_queue,
							  TORQUE_REQUEST_QUEUE_NAME,
							  TORQUE_REQUEST_QUEUE_ITEM_SIZE,
							  torque_request_queue_mem,
							  sizeof(torque_request_queue_mem));
	}

	return ret;
}

/**
 * @brief 	Terminate CAN transmit thread
 * 
 * @return 	See ThreadX return codes
 */
UINT can_tx_thread_terminate()
{
	return tx_thread_terminate(&can_tx_thread);
}


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
		// torque_request_message_t message;
		ULONG torque_request = 0;
		UINT ret = tx_queue_receive(&torque_request_queue, &torque_request, TX_WAIT_FOREVER);

		if (ret != TX_SUCCESS) 
		{
			// should never happen but handle it for safety
			continue;
		}

#if INVERTER_SPEED_MODE
		// map torque to speed request and send to inverter through CAN
		// UINT speed = foo(message.value);
		UINT speed_request = torque_request * 10;
		pm100_speed_request(speed_request);
#else
	#if !(INVERTER_DISABLE_TORQUE_REQUESTS)
		
		// send the torque request to inverter through CAN
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
