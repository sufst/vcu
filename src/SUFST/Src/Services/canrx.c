#include "canrx.h"

#include <can_c.h>
#include <can_s.h>

/*
 * internal function prototypes
 */
static void canrx_thread_entry(ULONG input);
static void process_broadcast(canrx_context_t* canrx_ptr,
                              const rtcan_msg_t* msg_ptr);

/**
 * @brief   Initialises the CANRX service
 *
 * @param[in]   canrx_ptr       CANRX context
 * @param[in]   stack_pool_ptr  Memory pool for service thread stack
 * @param[in]   rtcan_c_ptr     RTCAN C instance for receiving broadcasts
 * @param[in]   rtcan_s_ptr     RTCAN S instance for sending precharge cmd
 * @param[in]   config_ptr      Configuration
 */
status_t canrx_init(canrx_context_t* canrx_ptr,
                    rtcan_handle_t* rtcan_c_ptr,
                    rtcan_handle_t* rtcan_s_ptr,
                    TX_BYTE_POOL* stack_pool_ptr,
                    const config_canrx_t* config_ptr)
{
    canrx_ptr->config_ptr = config_ptr;
    canrx_ptr->rtcan_c_ptr = rtcan_c_ptr;
    canrx_ptr->rtcan_s_ptr = rtcan_s_ptr;
    canrx_ptr->error = CANRX_ERROR_NONE;
    canrx_ptr->broadcasts_valid = false;

    status_t status = STATUS_OK;

    // create service thread
    void* stack_ptr = NULL;
    UINT tx_status = tx_byte_allocate(stack_pool_ptr,
                                      &stack_ptr,
                                      config_ptr->thread.stack_size,
                                      TX_NO_WAIT);

    if (tx_status == TX_SUCCESS)
    {
        tx_status = tx_thread_create(&canrx_ptr->thread,
                                     (CHAR*) config_ptr->thread.name,
                                     canrx_thread_entry,
                                     (ULONG) canrx_ptr,
                                     stack_ptr,
                                     config_ptr->thread.stack_size,
                                     config_ptr->thread.priority,
                                     config_ptr->thread.priority,
                                     TX_NO_TIME_SLICE,
                                     TX_AUTO_START);
    }

    // create CAN receive queue
    if (tx_status == TX_SUCCESS)
    {
        tx_status = tx_queue_create(&canrx_ptr->can_c_rx_queue,
                                    NULL,
                                    TX_1_ULONG,
                                    canrx_ptr->can_rx_queue_mem,
                                    sizeof(canrx_ptr->can_rx_queue_mem));
    }

    if (tx_status == TX_SUCCESS)
    {
        tx_status = tx_queue_create(&canrx_ptr->can_s_rx_queue,
                                    NULL,
                                    TX_1_ULONG,
                                    canrx_ptr->can_rx_queue_mem,
                                    sizeof(canrx_ptr->can_rx_queue_mem));
    }

    // create state mutex
    if (tx_status == TX_SUCCESS)
    {
        tx_status = tx_mutex_create(&canrx_ptr->state_mutex, NULL, TX_INHERIT);
    }

    if (tx_status != TX_SUCCESS)
    {
        status = STATUS_ERROR;
    }

    // check all ok
    if (status != STATUS_OK)
    {
        tx_thread_terminate(&canrx_ptr->thread);
    }

    // turn off power
    HAL_GPIO_WritePin(STATUS_GPIO_Port, // This pin used to be called STATUS
                      STATUS_Pin,
                      GPIO_PIN_RESET);

    return status;
}

/**
 * @brief   CANRX service thread entry function
 *
 * @details This thread is responsible for receiving and processing broadcast
 *          messages from the CANRX
 */
void canrx_thread_entry(ULONG input)
{
    canrx_context_t* canrx_ptr = (canrx_context_t*) input;
    const config_canrx_t* config_ptr = canrx_ptr->config_ptr;

    // set up RTCAN subscriptions
    uint32_t can_c_subscriptions[0];

    for (uint32_t i = 0;
         i < sizeof(can_c_subscriptions) / sizeof(can_c_subscriptions[0]);
         i++)
    {
        rtcan_status_t status = rtcan_subscribe(canrx_ptr->rtcan_c_ptr,
                                                can_c_subscriptions[i],
                                                &canrx_ptr->can_c_rx_queue);

        if (status != RTCAN_OK)
        {
            // TODO: update broadcast states with error
            tx_thread_terminate(&canrx_ptr->thread);
        }
    }

    uint32_t can_s_subscriptions[] = {CAN_S_MSGID_0_X201_FRAME_ID};

    for (uint32_t i = 0;
         i < sizeof(can_s_subscriptions) / sizeof(can_s_subscriptions[0]);
         i++)
    {
        rtcan_status_t status = rtcan_subscribe(canrx_ptr->rtcan_c_ptr,
                                                can_s_subscriptions[i],
                                                &canrx_ptr->can_s_rx_queue);

        if (status != RTCAN_OK)
        {
            // TODO: update broadcast states with error
            tx_thread_terminate(&canrx_ptr->thread);
        }
    }

    // process incoming messages, or timeout
    while (1)
    {
        UINT status = TX_QUEUE_ERROR;
        rtcan_msg_t* msg_ptr = NULL;
        UINT can_s_status
            = tx_queue_receive(&canrx_ptr->can_s_rx_queue,
                               &msg_ptr,
                               config_ptr->broadcast_timeout_ticks);

        if (can_s_status == TX_SUCCESS && msg_ptr != NULL)
        {
            canrx_ptr->broadcasts_valid = true;
            process_broadcast(canrx_ptr, msg_ptr);
            rtcan_msg_consumed(canrx_ptr->rtcan_s_ptr, msg_ptr);
            status = TX_SUCCESS;
        }

        msg_ptr = NULL;
        UINT can_c_status
            = tx_queue_receive(&canrx_ptr->can_c_rx_queue,
                               &msg_ptr,
                               config_ptr->broadcast_timeout_ticks);

        if (can_c_status == TX_SUCCESS && msg_ptr != NULL)
        {
            canrx_ptr->broadcasts_valid = true;
            process_broadcast(canrx_ptr, msg_ptr);
            rtcan_msg_consumed(canrx_ptr->rtcan_c_ptr, msg_ptr);
            status = TX_SUCCESS;
        }

        if (can_s_status == TX_QUEUE_EMPTY && can_c_status == TX_QUEUE_EMPTY)
        {
            canrx_ptr->broadcasts_valid = false;
            LOG_ERROR("CANRX recieving timed out\n");
        }
        else if (status == TX_SUCCESS)
        {
            canrx_ptr->broadcasts_valid = true;
            process_broadcast(canrx_ptr, msg_ptr);
            rtcan_msg_consumed(canrx_ptr->rtcan_c_ptr, msg_ptr);
        }
        else
        {
            LOG_ERROR("CANRX other error\n");
        }
    }
}

/**
 * @brief   Processes incoming broadcast messages
 *
 * @param[in]   canrx_ptr   CANRX context
 * @param[in]   msg_ptr     Incoming message
 */
void process_broadcast(canrx_context_t* canrx_ptr, const rtcan_msg_t* msg_ptr)
{
    switch (msg_ptr->identifier)
    {

    case CAN_S_MSGID_0_X201_FRAME_ID:
    {
        can_s_msgid_0_x201_unpack(&canrx_ptr->bms_pack,
                                  msg_ptr->data,
                                  msg_ptr->length);

        break;
    }

    default:
        break;
    }
}
