/***************************************************************************
 * @file   rtcan.c
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief  RTOS wrapper around CAN bus
 ***************************************************************************/

#include "rtcan.h"

#include <stdbool.h>
#include <stdint.h>

#include <memory.h>

#define RTCAN_THREAD_NAME       "RTCAN Thread"
#define RTCAN_THREAD_STACK_SIZE 512 // TODO: this needs to be profiled

/**
 * internal functions
 */
static rtcan_status_t create_status(rtcan_handle_t* rtcan_ptr);
static bool no_errors(rtcan_handle_t* rtcan_ptr);
static void rtcan_thread_entry(ULONG input);

static rtcan_status_t transmit_internal(rtcan_handle_t* rtcan_ptr,
                                        uint32_t identifier,
                                        const uint8_t* data_ptr,
                                        uint32_t data_length);

/**
 * @brief       Initialises the RTCAN instance
 *
 * @details     The CAN instance of the handle should not be used by any other
 *              part of the system
 *
 * @param[in]   rtcan_ptr       Pointer to RTCAN context
 * @param[in]   hcan            CAN handle
 * @param[in]   stack_pool      Memory pool to allocate stack memory from
 */
rtcan_status_t rtcan_init(rtcan_handle_t* rtcan_ptr,
                          CAN_HandleTypeDef* hcan,
                          ULONG priority,
                          TX_BYTE_POOL* stack_pool)
{
    rtcan_ptr->hcan = hcan;
    rtcan_ptr->err = RTCAN_ERROR_NONE;

    // thread
    void* stack_ptr = NULL;

    UINT tx_status = tx_byte_allocate(stack_pool,
                                      &stack_ptr,
                                      RTCAN_THREAD_STACK_SIZE,
                                      TX_NO_WAIT);

    if (tx_status != TX_SUCCESS)
    {
        rtcan_ptr->err |= RTCAN_ERROR_INIT;
    }

    if (no_errors(rtcan_ptr))
    {
        tx_status = tx_thread_create(&rtcan_ptr->thread,
                                     RTCAN_THREAD_NAME,
                                     rtcan_thread_entry,
                                     (ULONG) rtcan_ptr,
                                     stack_ptr,
                                     RTCAN_THREAD_STACK_SIZE,
                                     priority,
                                     priority,
                                     TX_NO_TIME_SLICE,
                                     TX_DONT_START);

        if (tx_status != TX_SUCCESS) // must have been caused by invalid arg
        {
            rtcan_ptr->err |= RTCAN_ERROR_INIT;
        }
    }

    // transmit queue
    if (no_errors(rtcan_ptr))
    {
        tx_status = tx_queue_create(&rtcan_ptr->tx_queue,
                                    "RTCAN Transmit Queue",
                                    RTCAN_TX_QUEUE_ITEM_SIZE,
                                    rtcan_ptr->tx_queue_mem,
                                    RTCAN_TX_QUEUE_SIZE);

        if (tx_status != TX_SUCCESS)
        {
            rtcan_ptr->err |= RTCAN_ERROR_INIT;
        }
    }

    // transmit mailbox semaphore
    if (no_errors(rtcan_ptr))
    {
        const uint32_t mailbox_size
            = sizeof(rtcan_ptr->hcan->Instance->sTxMailBox)
              / sizeof(CAN_TxMailBox_TypeDef);

        tx_status = tx_semaphore_create(&rtcan_ptr->sem, NULL, mailbox_size);

        if (tx_status != TX_SUCCESS)
        {
            rtcan_ptr->err |= RTCAN_ERROR_INTERNAL;
        }
    }

    // start peripheral
    if (no_errors(rtcan_ptr))
    {
        HAL_StatusTypeDef hal_status = HAL_CAN_Start(rtcan_ptr->hcan);

        if (hal_status != HAL_OK)
        {
            rtcan_ptr->err |= RTCAN_ERROR_INIT;
        }
    }

    return create_status(rtcan_ptr);
}

/**
 * @brief   Starts the RTCAN service
 */
rtcan_status_t rtcan_start(rtcan_handle_t* rtcan_ptr)
{
    UINT tx_status = tx_thread_resume(&rtcan_ptr->thread);

    if (tx_status != TX_SUCCESS)
    {
        rtcan_ptr->err |= RTCAN_ERROR_INIT;
    }

    return create_status(rtcan_ptr);
}

/**
 * @brief       Transmits a CAN message using the RTCAN service
 *
 * @details     The message is queued for transmission in a FIFO buffer, and the
 *              contents of the message is copied (!) to the buffer
 *
 * @param[in]   rtcan_ptr       Pointer to RTCAN context
 * @param[in]   msg_ptr         Pointer to message to transmit
 */
rtcan_status_t rtcan_transmit(rtcan_handle_t* rtcan_ptr, rtcan_msg_t* msg_ptr)
{
    UINT tx_status
        = tx_queue_send(&rtcan_ptr->tx_queue, (void*) msg_ptr, TX_NO_WAIT);

    if (tx_status != TX_SUCCESS)
    {
        rtcan_ptr->err |= RTCAN_ERROR_QUEUE_FULL;
    }

    return create_status(rtcan_ptr);
}

/**
 * @brief       Retrieves the error code
 *
 * @param[in]   rtcan_ptr   Pointer to RTCAN context
 */
uint32_t rtcan_get_error(rtcan_handle_t* rtcan_ptr)
{
    return rtcan_ptr->err;
}

/**
 * @brief       Entry function for RTCAN service thread
 *
 * @param[in]   input   Pointer to RTCAN context
 */
static void rtcan_thread_entry(ULONG input)
{
    rtcan_handle_t* rtcan_ptr = (rtcan_handle_t*) input;

    while (1)
    {
        const rtcan_msg_t message;
        UINT tx_status = tx_queue_receive(&rtcan_ptr->tx_queue,
                                          (void*) &message,
                                          TX_WAIT_FOREVER);

        if (tx_status == TX_SUCCESS)
        {
            (void) transmit_internal(rtcan_ptr,
                                     message.identifier,
                                     message.data,
                                     message.length);
        }
        else
        {
            // TODO: handle error
        }
    }
}

/**
 * @brief       Internal transmit for RTCAN service thread
 *
 * @param[in]   rtcan_ptr       Pointer to RTCAN context
 * @param[in]   identifier      CAN standard identifier
 * @param[in]   data_ptr        Pointer to data to transmit
 * @param[in]   data_length     Length of data to transmit
 */
static rtcan_status_t transmit_internal(rtcan_handle_t* rtcan_ptr,
                                        uint32_t identifier,
                                        const uint8_t* data_ptr,
                                        uint32_t data_length)
{
    if ((data_ptr == NULL) || (data_length == 0U))
    {
        rtcan_ptr->err |= RTCAN_ERROR_ARG;
    }

    if (tx_semaphore_get(&rtcan_ptr->sem, TX_WAIT_FOREVER) != TX_SUCCESS)
    {
        rtcan_ptr->err |= RTCAN_ERROR_INTERNAL;
    }

    if (no_errors(rtcan_ptr))
    {
        // create message
        CAN_TxHeaderTypeDef header = {
            .IDE = CAN_ID_STD,
            .RTR = CAN_RTR_DATA,
            .DLC = data_length,
            .StdId = identifier,
        };

        // send it
        uint32_t tx_mailbox;

        HAL_StatusTypeDef hal_status = HAL_CAN_AddTxMessage(rtcan_ptr->hcan,
                                                            &header,
                                                            data_ptr,
                                                            &tx_mailbox);

        if (hal_status != HAL_OK)
        {
            rtcan_ptr->err |= RTCAN_ERROR_INTERNAL;
        }
    }

    return create_status(rtcan_ptr);
}

/**
 * @brief       Returns true if the RTCAN instance has encountered an error
 *
 * @param[in]   rtcan_ptr   Pointer to RTCAN context
 */
static bool no_errors(rtcan_handle_t* rtcan_ptr)
{
    return (rtcan_ptr->err == RTCAN_ERROR_NONE);
}

/**
 * @brief       Create a status code based on the current error state
 *
 * @param[in]   rtcan_ptr   Pointer to RTCAN context
 */
static rtcan_status_t create_status(rtcan_handle_t* rtcan_ptr)
{
    return (no_errors(rtcan_ptr)) ? RTCAN_OK : RTCAN_ERROR;
}