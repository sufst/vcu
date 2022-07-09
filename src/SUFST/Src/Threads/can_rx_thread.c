/***************************************************************************
 * @file   can_rx_thread.c
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-05-15
 * @brief  CAN receive thread implementation
 ***************************************************************************/

#include "can_rx_thread.h"

#include "config.h"
#include "tx_api.h"

#include "can_device_state.h"

#define CAN_RX_THREAD_STACK_SIZE           1024
#define CAN_RX_THREAD_PREEMPTION_THRESHOLD CAN_RX_THREAD_PRIORITY
#define CAN_RX_THREAD_NAME                 "CAN Rx Thread"
#define CAN_RX_SEMAPHORE_NAME              "CAN Rx Semaphore"

/**
 * @brief CAN receive thread
 */
TX_THREAD can_rx_thread;

/**
 * @brief   CAN receive semaphore
 *
 * @details Signalled by ISR to activate CAN receive thread
 */
TX_SEMAPHORE can_rx_semaphore;

/*
 * function prototypes
 */
void can_rx_thread_entry(ULONG thread_input);

/**
 * @brief 		Create CAN receive thread
 *
 * @param[in]	stack_pool_ptr 	Pointer to start of application stack area
 *
 * @return 		See ThreadX return codes
 */
UINT can_rx_thread_create(TX_BYTE_POOL* stack_pool_ptr)
{
    // create thread
    VOID* thread_stack_ptr;

    UINT ret = tx_byte_allocate(stack_pool_ptr,
                                &thread_stack_ptr,
                                CAN_RX_THREAD_STACK_SIZE,
                                TX_NO_WAIT);

    if (ret == TX_SUCCESS)
    {
        ret = tx_thread_create(&can_rx_thread,
                               CAN_RX_THREAD_NAME,
                               can_rx_thread_entry,
                               0,
                               thread_stack_ptr,
                               CAN_RX_THREAD_STACK_SIZE,
                               CAN_RX_THREAD_PRIORITY,
                               CAN_RX_THREAD_PREEMPTION_THRESHOLD,
                               TX_NO_TIME_SLICE,
                               TX_AUTO_START);
    }

    // create semaphore
    if (ret == TX_SUCCESS)
    {
        ret = tx_semaphore_create(&can_rx_semaphore, CAN_RX_SEMAPHORE_NAME, 0);
    }

    return ret;
}

/**
 * @brief 	Terminate CAN receive thread
 *
 * @return 	See ThreadX return codes
 */
UINT can_rx_thread_terminate()
{
    return tx_thread_terminate(&can_rx_thread);
}

/**
 * @brief       Thread entry function for CAN receive dispatch thread
 *
 * @param[in]   thread_input    (Unused) thread input
 */
void can_rx_thread_entry(ULONG thread_input)
{
    // not using input, prevent compiler warning
    (void) thread_input;

    // start CAN receive
    HAL_FDCAN_Start(&hfdcan1);
    HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);

    // loop forever
    while (1)
    {
        // wait for CAN receive event
        if (tx_semaphore_get(&can_rx_semaphore, TX_WAIT_FOREVER) == TX_SUCCESS)
        {
            // read data
            can_msg_t message;
            HAL_StatusTypeDef hal_status
                = HAL_FDCAN_GetRxMessage(&hfdcan1,
                                         FDCAN_RX_FIFO0,
                                         &message.rx_header,
                                         message.data);

            // reactivate notification
            HAL_FDCAN_ActivateNotification(&hfdcan1,
                                           FDCAN_IT_RX_FIFO0_NEW_MESSAGE,
                                           0);

            // update device state
            if (hal_status == HAL_OK)
            {
                can_update_device_state(&message);
            }
        }
    }
}

/**
 * @brief      CAN Rx Fifo 0 message callback
 *
 * @param[in]  hfdcan  FDCAN handle
 */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef* hfdcan, uint32_t RxFifo0ITs)
{
    // if new message in buffer, signal receive thread to wake
    if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET)
    {
        tx_semaphore_put(&can_rx_semaphore);
    }
    else
    {
        Error_Handler();
    }
}