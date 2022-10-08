/***************************************************************************
 * @file   can_rx_thread.h
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-05-15
 * @brief  CAN receive thread definitions and prototypes
 ***************************************************************************/

#ifndef CAN_RX_THREAD_H
#define CAN_RX_THREAD_H

#include "tx_api.h"

#include "can.h"

UINT can_rx_thread_create(TX_BYTE_POOL* stack_pool_ptr);
UINT can_rx_thread_terminate();
void HAL_FDCAN_RxFifo0Callback(CAN_HandleTypeDef* hfdcan, uint32_t RxFifo0ITs);

#endif