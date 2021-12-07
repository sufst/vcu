/***************************************************************************
 * @file   messaging_system.h
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2021-11-30
 * @brief  Inter-thread messaging system function prototypes and definitions
 ***************************************************************************/

#ifndef MESSAGING_SYSTEM_H
#define MESSAGING_SYSTEM_H

#include "tx_api.h"

#define TORQUE_REQUEST_QUEUE_MESSAGE_SIZE	2
#define TORQUE_REQUEST_QUEUE_SIZE			100
#define TORQUE_REQUEST_QUEUE_NAME			"Torque Request Queue"

extern TX_QUEUE torque_request_queue;

// function prototypes
UINT message_system_init();

#endif
