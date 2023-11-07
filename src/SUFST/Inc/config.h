/***************************************************************************
 * @file    config.h
 * @author  Tim Brewis (tab1g19@soton.ac.uk)
 * @brief   System configuration
 * @details Related configuration parameters are grouped as a struct
 * @note    The autoformatter is disabled for config.h and config.c
***************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

#include <adc.h>
#include <gpio.h>
#include <tx_api.h>
#include <stdint.h>
#include <stdbool.h>

#include "torque_map_funcs.h"

/**
 * @brief  Threads
 */
typedef struct {
    uint32_t priority;                      // thread priority
    uint32_t stack_size;                    // stack size
    const char* name;                       // name
} config_thread_t;

/**
 * @brief   System critical signals
 */
typedef struct {
    ADC_HandleTypeDef* hadc;                // ADC handle
    uint16_t min_adc;                       // minimum expected ADC reading
    uint16_t max_adc;                       // maximum expected ADC reading
    uint16_t min_mapped;                    // minimum mapped reading
    uint16_t max_mapped;                    // maximum mapped reading
    float outside_bounds_fraction;          // fraction of mapped range defining out of bounds signal
} config_scs_t;

/**
 * @brief   Control
 */
typedef struct {
    config_thread_t thread;                 // control thread config
    bool r2d_requires_brake;                // whether or not the brake needs to be pressed for R2D activation
    uint32_t ts_ready_timeout_ticks;        // ticks after which waiting for TS ready times out
    uint32_t ts_ready_poll_ticks;           // how often to poll input when waiting for TS ready
    uint32_t precharge_timeout_ticks;       // ticks after which waiting for precharge times out
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
 * @brief   APPS
 */
typedef struct {
    config_scs_t apps_1_scs;                // SCS configuration for first APPS signal
    config_scs_t apps_2_scs;                // SCS configuration for second APPS signal
    uint32_t max_discrepancy;               // maximum discrepancy between APPS readings
} config_apps_t;

/**
 * @brief   BPS
 */
typedef struct {
    config_scs_t scs;                       // SCS configuration
    float fully_pressed_fraction;           // threshold above which considered 'fully pressed'
} config_bps_t;

/**
 * @brief   Ready to drive speaker
 */
typedef struct {
    uint32_t active_ticks;                  // ticks for which RTDS sounds
    GPIO_TypeDef* port;                     // port for pin driving RTDS
    uint16_t pin;                           // pin driving RTDS
} config_rtds_t;

/**
 * @brief   Torque map
 * 
 * @note    All torque is represented as Nm * 10
 */
typedef struct {
    torque_map_func_e function;             // mapping function
    uint16_t input_max;                     // maximum input value (range must be zero to max)
    uint16_t output_max;                    // maximum output value (Nm * 10)
    float deadzone_fraction;                // fraction of input range for deadzone
} config_torque_map_t;

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
    config_apps_t apps;
    config_bps_t bps;
    config_ctrl_t ctrl;
    config_rtds_t rtds;
    config_torque_map_t torque_map;
    config_canbc_t canbc;
} config_t;

/*
 * public functions
 */
const config_t* config_get();









/***************************************************************************
 * 
 * NOTE: MIGRATION OF OLD CONFIG SYSTEM BELOW IN PROGRESS!
 * 
 ***************************************************************************/

#ifndef COMPETITION_MODE
    #define COMPETITION_MODE		        0
#endif

/***************************************************************************
 * ready-to-drive
 ***************************************************************************/

#define READY_TO_DRIVE_OVERRIDE		        1		// set to 1 to use the 'USER' button as the ready-to-drive signal
#define READY_TO_DRIVE_IGNORE_BPS           1
#define READY_TO_DRIVE_BUZZER_TIME	        2500	// in ms

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
