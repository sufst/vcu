/***************************************************************************
 * @file   init.h
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-05-20
 * @brief  Initialisation function prototypes
 ***************************************************************************/

#ifndef INIT_H
#define INIT_H

#include "tx_api.h"

void init_pre_rtd(TX_BYTE_POOL* stack_pool_ptr);
void init_post_rtd();

#endif