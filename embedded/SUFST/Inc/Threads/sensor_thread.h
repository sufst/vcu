/***************************************************************************
 * @file   sensor_thread.h
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2021-11-30
 * @brief  Sensor thread
 ***************************************************************************/

#ifndef SENSOR_THREAD_H
#define SENSOR_THREAD_H

#include "tx_api.h"

UINT sensor_thread_create(TX_BYTE_POOL* stack_pool_ptr);
UINT sensor_thread_terminate();

#endif
