/***************************************************************************
 * @file   config.h
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2022-02-08
 * @brief  System configuration
 *
 * @note   See config_rules.c which checks these parameters are valid
***************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

#include "tx_api.h"

/***************************************************************************
 * competition mode -- set to 1 to apply strict checks on config
 ***************************************************************************/

#define COMPETITION_MODE			0

/***************************************************************************
 * ready-to-drive
 ***************************************************************************/

#define READY_TO_DRIVE_OVERRIDE		1		// set to 0 to use the 'USER' button as the ready-to-drive signal
#define READY_TO_DRIVE_BUZZER_TIME	2500	// in ms

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
 * CAN
 ***************************************************************************/

#define CAN_TORQUE_REQUEST_TIMEOUT	100		// in ms
#define CAN_EEPROM_MAX_RETRY		10		// maximum number of retry attempts
#define CAN_EEPROM_RETRY_DELAY		100		// in ms

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

#endif
