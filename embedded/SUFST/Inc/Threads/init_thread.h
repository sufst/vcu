/***************************************************************************
 * @file   init_thread.h
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-06-08
 * @brief  Initialisation thread
 ***************************************************************************/

#ifndef INIT_THREAD_H
#define INIT_THREAD_H

#include "tx_api.h"

UINT init_thread_create(TX_BYTE_POOL* stack_pool_ptr);

#endif