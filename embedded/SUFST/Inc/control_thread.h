/***************************************************************************
 * @file   control_thread.h
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2021-11-09
 * @brief  Control thread definitions and prototypes
 ***************************************************************************/

#ifndef CONTROL_THREAD_H
#define CONTROL_THREAD_H

#include "tx_api.h"

// LED thread definitions
#define CONTROL_THREAD_STACK_SIZE			1024
#define CONTROL_THREAD_PRIORITY				3
#define CONTROL_THREAD_PREEMPTION_THRESHOLD CONTROL_THREAD_PRIORITY
#define CONTROL_THREAD_NAME					"Control Thread"

// expose thread instance to files including this header
extern TX_THREAD control_thread;

// function prototypes
void control_thread_entry(ULONG thread_input);

#endif
