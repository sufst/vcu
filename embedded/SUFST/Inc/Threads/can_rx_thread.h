/***************************************************************************
 * @file   can_rx_thread.h
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
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
 * @brief CAN receive thread
 */
extern TX_THREAD can_rx_thread;

/*
 * function prototypes
 */
void can_rx_thread_entry(ULONG thread_input);
UINT can_rx_init();
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs);

#endif