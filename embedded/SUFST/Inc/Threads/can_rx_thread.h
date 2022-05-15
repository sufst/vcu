/***************************************************************************
 * @file   can_rx_thread.h
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2022-05-15
 * @brief  CAN receive thread definitions and prototypes
 ***************************************************************************/

#ifndef CAN_RX_THREAD_H
#define CAN_RX_THREAD_H

#include "tx_api.h"
#include "config.h"

#include "can_device_state.h"
#include "fdcan.h"

#define CAN_RX_THREAD_STACK_SIZE            1024
#define CAN_RX_THREAD_PREEMPTION_THRESHOLD  CAN_RX_THREAD_PRIORITY
#define CAN_RX_THREAD_NAME                  "CAN Rx Thread"

/**
 * @brief   CAN receive thread dispatch task
 * 
 * @details Receive CAN messages should not be processed in the IRQ handler, instead they
 *          should be dispatched to the CAN receive thread for asynchronous processing.
 *          This struct represents a CAN message processing task to be dispatched to the
 *          CAN receive thread.
 * 
 *          Usage:
 *          - Create new tasks using can_rx_dispatch_task_create()
 *          - Dispatch them for asynchronous processing using can_rx_dispatch_task_async()
 *          - Release tasks on failed dispatch using can_rx_dispatch_task_release()
 * 
 *          Important:
 *          - Failure to release a task if it was not dispatched will result in a memory leak
 *          - It is most efficient to first create a task, then have the HAL copy the CAN
 *            message directly into the can_msg_t field of the task struct
 */
typedef struct {

    /**
     * @brief CAN message
     */
    can_msg_t message;

} can_rx_dispatch_task_t;

/**
 * @brief CAN receive thread
 */
extern TX_THREAD can_rx_thread;

/**
 * @brief CAN receive data mutex
 */
extern TX_MUTEX can_rx_data_mutex;

// function prototypes
void can_rx_thread_entry(ULONG thread_input);
UINT can_rx_dispatch_init();
UINT can_rx_dispatch_task_create(can_rx_dispatch_task_t** dispatch_ptr);
UINT can_rx_dispatch_task_release(can_rx_dispatch_task_t* dispatch_ptr);
UINT can_rx_dispatch_task_async(can_rx_dispatch_task_t* dispatch_ptr);
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs);

#endif