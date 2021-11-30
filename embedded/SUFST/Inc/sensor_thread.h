/***************************************************************************
 * @file   sensor_thread.h
 * @author Cosmin-Andrei Tamas (cat1g19@soton.ac.uk)
 * @date   2021-11-30
 * @brief  Sensor thread
 ***************************************************************************/

#ifndef SENSOR_THREAD_H
#define SENSOR_THREAD_H

#include "tx_api.h"

// LED thread definitions
#define SENSOR_THREAD_STACK_SIZE				1024
#define SENSOR_THREAD_PRIORITY					5
#define SENSOR_THREAD_PREEMPTION_THRESHOLD		LED_THREAD_PRIORITY
#define SENSOR_THREAD_NAME						"Sensor Thread"

// expose thread instance to files including this header
extern TX_THREAD sensor_thread;

// function prototypes
void sensor_thread_entry(ULONG thread_input);

#endif
