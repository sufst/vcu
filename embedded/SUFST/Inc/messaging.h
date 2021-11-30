/***************************************************************************
 * @file   messaging.h
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2021-11-30
 * @brief  Inter-thread messaging system function prototypes and definitions
 ***************************************************************************/

#ifndef MESSAGING_H
#define MESSAGING_H

#include "tx_api.h"

#define SENSOR_QUEUE_MESSAGE_SIZE	4
#define SENSOR_QUEUE_SIZE			100
#define SENSOR_QUEUE_NAME			"Sensor Queue"

extern TX_QUEUE sensor_message_queue;

// function prototypes
UINT message_system_init();

#endif
