/***************************************************************************
 * @file   control_thread.h
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2021-11-09
 * @brief  Control thread definitions and prototypes
 ***************************************************************************/

#ifndef CONTROL_THREAD_H
#define CONTROL_THREAD_H

#include "tx_api.h"

extern TX_QUEUE apps_input_queue;

UINT control_thread_create(TX_BYTE_POOL* stack_pool_ptr);
UINT control_thread_start();
UINT control_thread_terminate();

#endif
