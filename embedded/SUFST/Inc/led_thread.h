/***************************************************************************
 * @file   led_thread.h
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2021-11-09
 * @brief  LED thread definitions and prototypes
 ***************************************************************************/

#ifndef LED_THREAD_H
#define LED_THREAD_H

#include "tx_api.h"

// LED thread definitions
#define LED_THREAD_STACK_SIZE				1024
#define LED_THREAD_PRIORITY					5
#define LED_THREAD_PREEMPTION_THRESHOLD		LED_THREAD_PRIORITY
#define LED_THREAD_NAME						"LED Thread"

// expose thread instance to files including this header
extern TX_THREAD led_thread;

// function prototypes
void led_thread_entry(ULONG thread_input);

#endif
