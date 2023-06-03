/***************************************************************************
 * @file    config.h
 * @author  Tim Brewis (tab1g19@soton.ac.uk)
 * @brief   System configuration
 * @details Related configuration parameters are grouped as a struct
 * @note    The autoformatter is disabled for config.h and config.c
***************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

#include <tx_api.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief  Generic configuration for threads
 */
typedef struct {
    uint32_t priority;                      // thread priority
    uint32_t stack_size;                    // stack size
    const char* name;                       // name
} config_thread_t;

/**
 * @brief   Control
 */
typedef struct {
    config_thread_t thread;                 // control thread config
    bool r2d_requires_brake;                // whether or not the brake needs to be pressed for R2D activation
    uint32_t ts_ready_timeout_ticks;        // ticks after which waiting for TS ready times out
    uint32_t ts_ready_poll_ticks;           // how often to poll input when waiting for TS ready
    uint32_t precharge_timeout_ticks;       // ticks after which waiting for precharge times out
    uint32_t rtds_sound_ticks;              // ticks for which RTDS is active
    uint32_t ready_wait_led_toggle_ticks;   // ticks between toggling the TS on LED while waiting for 'TS ready' from relay controller
    uint32_t error_led_toggle_ticks;        // ticks between toggling TS on LED in activation error
} config_ctrl_t;

/**
 * @brief   Dash
 */
typedef struct {
    config_thread_t thread;                 // dash thread config
    uint32_t btn_active_ticks;              // ticks for which a button must be pressed for it to be considered 'activated'
    uint32_t btn_sample_ticks;              // ticks between sampling buttons
    bool vc_run_check;                      // whether or not the visual check should run
    uint32_t vc_led_on_ticks;               // number of ticks for which the visual check should last
    bool vc_all_leds_on;                    // whether or not the visual check turns on all LEDs, or just the VC LEDs
    uint32_t vc_stagger_ticks;              // ticks between turning on each visible check LED (set to zero to turn all on at once)
} config_dash_t;

/**
 * @brief   CAN broadcasting service
 */
typedef struct {
    config_thread_t thread;                 // CANBC thread config
    uint32_t broadcast_period_ticks;        // ticks between broadcasts
} config_canbc_t;

/**
 * @brief   VCU configuration
 * 
 * @details The intended usage is that the main VCU module loads an instance of
 *          this struct and initialises all other modules based on its state. 
 *          Theoretically all the individual modules could load the config, 
 *          but that makes them less portable / decoupled.
 */
typedef struct {
    config_dash_t dash;
    config_ctrl_t ctrl;
    config_canbc_t canbc;
} config_t;

const config_t* config_get();









/***************************************************************************
 * 
 * NOTE: MIGRATION OF OLD CONFIG SYSTEM BELOW IN PROGRESS!
 * 
 ***************************************************************************/

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
#define TS_CTRL_THREAD_PRIORITY		        2

#define TRACEX_ENABLE                       0
       // enable TraceX logging

#define DRIVER_CTRL_TICK_RATE               100 // times per second

/***************************************************************************
 * CAN / inverter
 ***************************************************************************/

#define SELECTED_DRIVER_PROFILE                 DRIVER_PROFILE_DEFAULT

#define INVERTER_SPEED_MODE                     0       // replace torque requests with speed requests
#define INVERTER_TORQUE_REQUEST_TIMEOUT	        100		// in ms

/***************************************************************************
 * sensors
 ***************************************************************************/

#define APPS_DISABLE_DIFF_CHECK             1       // disable check for discrepancy between APPS inputs
#define APPS_DISABLE_SCS_CHECK              1       // disable check for APPS ADC reading out of bounds

#define APPS_ADC_RESOLUTION                 16      // resolution of raw APPS input from ADC
#define APPS_MAX_DIFF_FRACTION              0.025f  // maximum allowable difference between APPS inputs as a fraction of scaled range
#define APPS_OUTSIDE_BOUNDS_FRACTION        0.01f   // fraction of full ADC range above/below ADC min/max considered 'out of bounds'

#define APPS_1_ADC_MIN                      0x600    //  minimum raw ADC reading for APPS  channel 1
#define APPS_2_ADC_MIN                      0x600    // ^                                ^ channel 2
#define APPS_1_ADC_MAX                      0x900    //  maximum raw ADC reading for APPS  channel 1
#define APPS_2_ADC_MAX                      0x900    // ^                                ^ channel 2

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
