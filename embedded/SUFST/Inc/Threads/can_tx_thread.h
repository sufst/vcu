/***************************************************************************
 * @file   can_tx_thread.h
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2021-11-09
 * @brief  CAN transmit thread definitions and prototypes
 ***************************************************************************/

#ifndef CAN_TX_THREAD_H
#define CAN_TX_THREAD_H

#include "tx_api.h"

extern TX_QUEUE torque_request_queue;

UINT can_tx_thread_init(TX_BYTE_POOL* stack_pool_ptr);
UINT can_tx_thread_terminate();

#endif
