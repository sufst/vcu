/***************************************************************************
 * @file   can_rx_thread.c
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-05-15
 * @brief  CAN receive thread implementation
 ***************************************************************************/

#include "can_rx_thread.h"
#include "tx_api.h"

#include "can_driver.h"

#define DISPATCH_POOL_NUM_ITEMS  10
#define DISPATCH_POOL_SIZE       (DISPATCH_POOL_NUM_ITEMS * sizeof(can_rx_dispatch_task_t) / sizeof(ULONG))
#define DISPATCH_POOL_NAME       "CAN Rx Dispatch Pool"

#define DISPATCH_QUEUE_NUM_ITEMS DISPATCH_POOL_NUM_ITEMS
#define DISPATCH_QUEUE_SIZE      (DISPATCH_QUEUE_NUM_ITEMS * sizeof(can_rx_dispatch_task_t*) / sizeof(ULONG))
#define DISPATCH_QUEUE_NAME      "CAN Rx Dispatch Queue"

/**
 * @brief CAN receive thread instance
 */
TX_THREAD can_rx_thread;

/**
 * @brief CAN receive async dispatch queue
 */
static TX_QUEUE dispatch_queue;

/**
 * @brief Memory area for async dispatch queue
 */
static ULONG dispatch_queue_mem[DISPATCH_QUEUE_SIZE];

/**
 * @brief CAN receive dispatch item pool
 */
static TX_BYTE_POOL dispatch_pool;

/**
 * @brief Memory area for CAN receive dispatch item pool
 */
static ULONG dispatch_pool_mem[DISPATCH_POOL_SIZE];

/**
 * @brief       Thread entry function for CAN receive dispatch thread
 * 
 * @param[in]   thread_input    (Unused) thread input
 */
void can_rx_thread_entry(ULONG thread_input)
{
    // not using input, prevent compiler warning
    (void) thread_input;

    // loop forever
    while (1)
    {
        // wait for dispatch task to enter receive queue
        can_rx_dispatch_task_t* dispatch_ptr;
        UINT status = tx_queue_receive(&dispatch_queue, (VOID*) &dispatch_ptr, TX_WAIT_FOREVER);

        // parse message
        if (status == TX_SUCCESS)
        {
            if (dispatch_ptr->parser != NULL)
            {
                dispatch_ptr->parser(dispatch_ptr->message, 0);
            }
        }

        // release task
        tx_byte_release(dispatch_ptr);
    }
}

/**
 * @brief  Initialise CAN receive dispatch service
 * 
 * @return TX_SUCCESS if successful, ThreadX error code otherwise 
 */
UINT can_rx_dispatch_init()
{
    // create memory pool for dispatch items
    UINT status = tx_byte_pool_create(&dispatch_pool, DISPATCH_POOL_NAME, dispatch_pool_mem, DISPATCH_POOL_SIZE);

    // create dispatch queue
    if (status == TX_SUCCESS)
    {   
        UINT message_size = TX_1_ULONG;
        status = tx_queue_create(&dispatch_queue, DISPATCH_QUEUE_NAME, message_size, dispatch_queue_mem, DISPATCH_QUEUE_SIZE);
    }

    return status;
}

/**
 * @brief       Create CAN dispatch task
 * 
 * @param[in]   task_ptr    Pointer to pointer to store allocated dispatch task 
 * 
 * @return      See: tx_byte_allocate()
 */
UINT can_rx_dispatch_task_create(can_rx_dispatch_task_t** task_ptr)
{
    return tx_byte_allocate(&dispatch_pool, (VOID**) task_ptr, sizeof(can_rx_dispatch_task_t), TX_NO_WAIT);
}

/**
 * @brief       Release CAN dispatch task
 * 
 * @details     Call this when the task has been completed, or when dispatch failed
 * 
 * @param[in]   task_ptr    Pointer to dispatch task
 * 
 * @return      See: tx_byte_release()
 */
UINT can_rx_dispatch_task_release(can_rx_dispatch_task_t* dispatch_ptr)
{
    return tx_byte_release(dispatch_ptr);
}

/**
 * @brief       Dispatch task for asynchronous processing in CAN receive thread
 * 
 * @param[in]   task_ptr    Pointer to dispatch task
 * 
 * @return      See: tx_queue_send()
 */
UINT can_rx_dispatch_task_async(can_rx_dispatch_task_t* task_ptr)
{
    return tx_queue_send(&dispatch_queue, (VOID**) &task_ptr, TX_NO_WAIT);
}