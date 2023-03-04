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

#define READY_TO_DRIVE_BUTTON_ENABLE		1		// set to 1 to use the 'USER' button as the ready-to-drive signal
#define READY_TO_DRIVE_CHECK_BPS            0
#define READY_TO_DRIVE_SPEAKER_TIME	        2500	// in ms

/***************************************************************************
 * fault state
 ***************************************************************************/

#define FAULT_STATE_LED_BLINK_RATE 	        2		// in Hz

/***************************************************************************
 * RTOS
 ***************************************************************************/

#define RTCAN_S_PRIORITY                    3   
#define RTCAN_C_PRIORITY                    2   // critical systems more important than sensors
#define CANBC_PRIORITY                      4   // broadcast data not critical to system operation
#define TS_CTRL_THREAD_PRIORITY		        2
#define DRIVER_CTRL_THREAD_PRIORITY		    2
#define INIT_THREAD_PRIORITY                0

#define TRACEX_ENABLE                       0
       // enable TraceX logging

#define DRIVER_CTRL_TICK_RATE               100  // times per second
#define CANBC_BROADCAST_PERIOD              100 // milliseconds

/***************************************************************************
 * CAN / inverter
 ***************************************************************************/

#define SELECTED_DRIVER_PROFILE                 DRIVER_PROFILE_DEFAULT

#define INVERTER_SPEED_MODE                     0       // replace torque requests with speed requests
#define INVERTER_TORQUE_REQUEST_TIMEOUT	        100		// in ms
#define INVERTER_EEPROM_MAX_RETRY		        10		// maximum number of retry attempts
#define INVERTER_EEPROM_RETRY_DELAY		        100		// in ms

#define CANBC_DRIVER_INPUTS_ID                  0x100   // CAN broadcast address for driver inputs

/***************************************************************************
 * sensors
 ***************************************************************************/

#define APPS_DISABLE_DIFF_CHECK             1       // disable check for discrepancy between APPS inputs
#define APPS_DISABLE_SCS_CHECK              1       // disable check for APPS ADC reading out of bounds

#define APPS_ADC_RESOLUTION                 16      // resolution of raw APPS input from ADC
#define APPS_SCALED_RESOLUTION              10      // scaled (truncated) APPS 

#define APPS_MAX_DIFF_FRACTION              0.025f  // maximum allowable difference between APPS inputs as a fraction of scaled range
#define APPS_OUTSIDE_BOUNDS_FRACTION        0.01f   // fraction of full ADC range above/below ADC min/max considered 'out of bounds'

#define APPS_1_ADC_MIN                      1540    //  minimum raw ADC reading for APPS  channel 1
#define APPS_2_ADC_MIN                      1540    // ^                                ^ channel 2
#define APPS_1_ADC_MAX                      2780    //  maximum raw ADC reading for APPS  channel 1
#define APPS_2_ADC_MAX                      2780    // ^                                ^ channel 2

/***************************************************************************
 * BPS - brake pressure sensor
 ***************************************************************************/

#define BPS_DISABLE_SCS_CHECK               1       // disable check for BPS ADC reading out of bounds

#define BPS_ADC_MIN                         200     // minimum raw ADC reading for BPS
#define BPS_ADC_MAX                         4000    // maximum raw ADC reading for BPS
#define BPS_SCALED_RESOLUTION               10      // resolution of scaled BPS input
#define BPS_FULLY_PRESSED_THRESHOLD         0.95f   // fraction of BPS full range beyond which it is considered to be fully pressed

/***************************************************************************
 * SCS - safety critical signals
 ***************************************************************************/

#define SCS_OUTSIDE_BOUNDS_FRACTION         0.05f   // fraction of full ADC range above/below ADC min/max considered 'out of bounds'

/***************************************************************************
 * testbenches
 ***************************************************************************/

// enable flags
#define RUN_APPS_TESTBENCH		            0		// APPS input from lookup table
#define RUN_FAULT_STATE_TESTBENCH	        0		// 'USER' button (after ready to drive) causes fault state

// testbench parameters
#define APPS_TESTBENCH_LAPS 	            1		// 1 for standing start only, 2+ to add flying laps

#endif
