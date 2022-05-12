/***************************************************************************
 * @file   can_thread.h
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2021-11-09
 * @brief  CAN thread definitions and prototypes
 ***************************************************************************/

#ifndef CAN_THREAD_H
#define CAN_THREAD_H

#include "tx_api.h"

// LED thread definitions
#define CAN_THREAD_STACK_SIZE			1024
#define CAN_THREAD_PREEMPTION_THRESHOLD	CAN_THREAD_PRIORITY
#define CAN_THREAD_NAME					"CAN Thread"

// expose thread instance to files including this header
extern TX_THREAD can_thread;

// function prototypes
void can_thread_entry(ULONG thread_input);

#endif
