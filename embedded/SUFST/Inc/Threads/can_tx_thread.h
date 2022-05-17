/***************************************************************************
 * @file   can_tx_thread.h
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2021-11-09
 * @brief  CAN transmit thread definitions and prototypes
 ***************************************************************************/

#ifndef CAN_TX_THREAD_H
#define CAN_TX_THREAD_H

#include "tx_api.h"

#define CAN_TX_THREAD_STACK_SIZE			1024
#define CAN_TX_THREAD_PREEMPTION_THRESHOLD	CAN_THREAD_PRIORITY
#define CAN_TX_THREAD_NAME					"CAN Tx Thread"

// expose thread instance to files including this header
extern TX_THREAD can_tx_thread;

// function prototypes
void can_tx_thread_entry(ULONG thread_input);

#endif
