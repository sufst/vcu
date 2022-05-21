/***************************************************************************
 * @file   fault_state_thread.h
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-03-15
 * @brief  Fault state thread definitions and prototypes
 ***************************************************************************/

#ifndef FAULT_STATE_THREAD_H
#define FAULT_STATE_THREAD_H

#include "tx_api.h"

UINT fault_state_thread_init();
void enter_fault_state();

#endif
