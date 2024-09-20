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
#include <usart.h>
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
     uint32_t schedule_ticks;                // number of ticks between runs of the control loop thread
     bool r2d_requires_brake;                // whether or not the brake needs to be pressed for R2D activation
     uint32_t ts_ready_timeout_ticks;        // ticks after which waiting for TS ready times out
     uint32_t ts_ready_poll_ticks;           // how often to poll input when waiting for TS ready
     uint32_t precharge_timeout_ticks;       // ticks after which waiting for precharge times out
     uint32_t ready_wait_led_toggle_ticks;   // ticks between toggling the TS on LED while waiting for 'TS ready' from relay controller
     uint32_t error_led_toggle_ticks;        // ticks between toggling TS on LED in activation error
     uint16_t apps_bps_high_threshold;       // apps reading to fault when brake also pressed
     uint16_t apps_bps_low_threshold;        // apps reading to recover from fault
     uint16_t fan_on_threshold;              // temperature at which to turn on the fan
     uint16_t fan_off_threshold;             // temperature at which to turn off the fan
     uint16_t bps_on_threshold;              // BPS reading to consider BPS 'on'
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
 * @brief   PM100DZ inverter
 */
typedef struct {
     config_thread_t thread;                 // service thread config
     uint32_t broadcast_timeout_ticks;       // maximum number of ticks to wait for a broadcast
     uint32_t torque_request_timeout_ticks;
     uint32_t precharge_timeout_ticks;
     uint8_t speed_mode;
} config_pm100_t;

/**
 * @brief   CAN broadcasting service
 */
typedef struct {
     config_thread_t thread;                 // CANBC thread config
     uint32_t broadcast_period_ticks;        // ticks between broadcasts
} config_canbc_t;

typedef struct
{
     config_thread_t thread;                 // thread config
     uint32_t blink_period_ticks;            // period to blink the LED
} config_heartbeat_t;

typedef struct
{
     config_thread_t thread;
     uint16_t period;
} config_tick_t;

/**
 * @brief log level
 */
typedef enum
{
     LOG_LEVEL_DEBUG,
     LOG_LEVEL_INFO,
     LOG_LEVEL_WARN,
     LOG_LEVEL_ERROR,
     LOG_LEVEL_FATAL,
     LOG_LEVEL_NONE = 0xFF
} config_log_level_t;

/**
 * @brief log service configuration
 */
typedef struct
{
     config_thread_t thread;
     config_log_level_t min_level;
     UART_HandleTypeDef *uart;
} config_log_t;

typedef struct
{
     uint8_t rtcan_s_priority;
     uint8_t rtcan_c_priority;
     uint8_t ts_ctrl_thread_priority;
     bool tracex_enable;
     uint16_t driver_ctrl_tick_rate;
} config_rtos_t;

typedef struct
{
     bool run_apps_testbench;
     bool run_fault_state_testbench;
     uint8_t apps_testbench_laps;
} config_testbenches;

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
     config_pm100_t pm100;
     config_tick_t tick;
     config_canbc_t canbc;
     config_heartbeat_t heartbeat;
     config_log_t log;
     config_rtos_t rtos;
     config_testbenches testbenches;
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

/***************************************************************************
 * RTOS
//  ***************************************************************************/

// #define RTCAN_S_PRIORITY                    3   
// #define RTCAN_C_PRIORITY                    2   // critical systems more important than sensors
// #define TS_CTRL_THREAD_PRIORITY		        2

// #define TRACEX_ENABLE                       0
//        // enable TraceX logging

// #define DRIVER_CTRL_TICK_RATE               100 // times per second

/***************************************************************************
 * CAN / inverter
 ***************************************************************************/

#define SELECTED_DRIVER_PROFILE                 DRIVER_PROFILE_DEFAULT

// #define INVERTER_SPEED_MODE                     0       // replace torque requests with speed requests
// #define INVERTER_TORQUE_REQUEST_TIMEOUT	        100		// in ms


// /***************************************************************************
//  * testbenches
//  ***************************************************************************/

// // enable flags
// #define RUN_APPS_TESTBENCH		            0		// APPS input from lookup table
// #define RUN_FAULT_STATE_TESTBENCH	        0		// 'USER' button (after ready to drive) causes fault state

// // testbench parameters
// #define APPS_TESTBENCH_LAPS 	            1		// 1 for standing start only, 2+ to add flying laps

#endif
