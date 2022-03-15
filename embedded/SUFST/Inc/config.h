/***************************************************************************
 * @file   config.h
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2022-02-08
 * @brief  System configuration
***************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

#include "tx_api.h"

/***************************************************************************
 * ready-to-drive
 ***************************************************************************/

#define READY_TO_DRIVE_OVERRIDE		1		// set to 0 to use the 'USER' button as the ready-to-drive signal
#define READY_TO_DRIVE_BUZZER_TIME	2		// in seconds

/***************************************************************************
 * fault state
 ***************************************************************************/

#define FAULT_STATE_LED_BLINK_RATE 	2		// in Hz

/***************************************************************************
 * thread priorities
 ***************************************************************************/

#define FAULT_STATE_THREAD_PRIORITY	0		// maximum priority
#define SENSOR_THREAD_PRIORITY		3
#define CONTROL_THREAD_PRIORITY		3
#define CAN_THREAD_PRIORITY			2

/***************************************************************************
 * sensors
 ***************************************************************************/

#define THROTTLE_INPUT_RESOLUTION	16		// resolution of ADC
#define THROTTLE_SCALED_RESOLUTION	10		// scaled resolution sent to control thread
#define THROTTLE_DEADZONE_FRACTION	0.01	// fractional dead-zone below which throttle always zero
#define THROTTLE_MAX_DIFF_FRACTION	0.05	// fractional maximum allowed difference between ADC readings
#define THROTTLE_ENABLE_DIFF_CHECK	1		// set to 1 to enable check for discrepancy between ADC readings
#define THROTTLE_ENABLE_DEADZONE	0		// set to 1 to enable dead-zone

/***************************************************************************
 * testbenches
 ***************************************************************************/

// enable flags
#define RUN_THROTTLE_TESTBENCH		1		// throttle input from lookup table
#define RUN_FAULT_STATE_TESTBENCH	1		// 'USER' button (after ready to drive) causes fault state

// testbench parameters
#define THROTTLE_TESTBENCH_LAPS 	4		// 1 for standing start only, 2+ to add flying laps

/***************************************************************************
 * safeguards
 ***************************************************************************/

// TODO: prevent illegal combinations of test benches from running (if any)
// TODO: if no test benches, make sure config values match the rules


#define NUM_TESTBENCHES_RUNNING		(RUN_THROTTLE_TESTBENCH \
									 +	RUN_FAULT_STATE_TESTBENCH)

#define TESTBENCHES_RUNNING			(NUM_TESTBENCHES_RUNNING > 0)


#endif
