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
 * competition mode 
 * -> defaults to 0 for debug
 * -> set to 1 automatically in release build
 * -> this enables strict checks on configuration 
 ***************************************************************************/

#ifndef COMPETITION_MODE
    #define COMPETITION_MODE		        0
#endif

/***************************************************************************
 * ready-to-drive
 ***************************************************************************/

#define READY_TO_DRIVE_OVERRIDE		        1		// set to 0 to use the 'USER' button as the ready-to-drive signal
#define READY_TO_DRIVE_BUZZER_TIME	        2500	// in ms

/***************************************************************************
 * fault state
 ***************************************************************************/

#define FAULT_STATE_LED_BLINK_RATE 	        2		// in Hz

/***************************************************************************
 * RTOS
 ***************************************************************************/

#define FAULT_STATE_THREAD_PRIORITY	        0		// maximum priority
#define SENSOR_THREAD_PRIORITY		        3
#define CONTROL_THREAD_PRIORITY		        3
#define CAN_THREAD_PRIORITY			        2
#define CAN_RX_THREAD_PRIORITY              4

#define TRACEX_ENABLE                       1       // enable TraceX

/***************************************************************************
 * CAN / inverter
 ***************************************************************************/

#define INVERTER_SPEED_MODE                     0       // replace torque requests with speed requests
#define INVERTER_TORQUE_REQUEST_TIMEOUT	        100		// in ms
#define INVERTER_EEPROM_MAX_RETRY		        10		// maximum number of retry attempts
#define INVERTER_EEPROM_RETRY_DELAY		        100		// in ms

#define INVERTER_BROADCAST_TEMPERATURE_1        0
#define INVERTER_BROADCAST_TEMPERATURE_2        0
#define INVERTER_BROADCAST_TEMPERATURE_3        0
#define INVERTER_BROADCAST_ANALOG_INPUTS        0
#define INVERTER_BROADCAST_DIGITAL_INPUTS       0
#define INVERTER_BROADCAST_MOTOR_POSITION       0
#define INVERTER_BROADCAST_CURRENTS             0
#define INVERTER_BROADCAST_VOLTAGES             0
#define INVERTER_BROADCAST_FLUX                 0
#define INVERTER_BROADCAST_INTERNAL_VOLTAGES    0
#define INVERTER_BROADCAST_INTERNAL_STATES      0
#define INVERTER_BROADCAST_FAULT_CODES          0
#define INVERTER_BROADCAST_TORQUE_TIMER         0
#define INVERTER_BROADCAST_MODULATION_FLUX      0
#define INVERTER_BROADCAST_FIRMWARE_INFO        0
#define INVERTER_BROADCAST_DIAGNOSTIC_DATA      0

/***************************************************************************
 * sensors
 ***************************************************************************/

#define THROTTLE_INPUT_RESOLUTION	        16		// resolution of ADC
#define THROTTLE_SCALED_RESOLUTION	        10		// scaled resolution sent to control thread
#define THROTTLE_DEADZONE_FRACTION	        0.01	// fractional dead-zone below which throttle always zero
#define THROTTLE_MAX_DIFF_FRACTION	        0.05	// fractional maximum allowed difference between ADC readings
#define THROTTLE_ENABLE_DIFF_CHECK	        1		// set to 1 to enable check for discrepancy between ADC readings
#define THROTTLE_ENABLE_DEADZONE	        0		// set to 1 to enable dead-zone

/***************************************************************************
 * testbenches
 ***************************************************************************/

// enable flags
#define RUN_THROTTLE_TESTBENCH		        1		// throttle input from lookup table
#define RUN_FAULT_STATE_TESTBENCH	        1		// 'USER' button (after ready to drive) causes fault state

// testbench parameters
#define THROTTLE_TESTBENCH_LAPS 	        4		// 1 for standing start only, 2+ to add flying laps

#endif
