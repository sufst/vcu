/***************************************************************************
 * @file   fault_state_thread.h
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2022-03-15
 * @brief  Fault state thread
 ***************************************************************************/

#ifndef FAULT_STATE_THREAD_H
#define FAULT_STATE_THREAD_H

#include "tx_api.h"

// LED thread definitions
#define FAULT_STATE_THREAD_STACK_SIZE				512
#define FAULT_STATE_THREAD_PRIORITY					0
#define FAULT_STATE_THREAD_PREEMPTION_THRESHOLD		FAULT_STATE_THREAD_PRIORITY
#define FAULT_STATE_THREAD_NAME						"Fault State Thread"

// expose thread instance to files including this header
extern TX_THREAD fault_state_thread;

// function prototypes
void fault_state_thread_entry(ULONG thread_input);

#endif
