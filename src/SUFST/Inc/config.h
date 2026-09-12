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
#include <stdbool.h>
#include <stdint.h>
#include <tx_api.h>
#include <usart.h>

#include "torque_map_funcs.h"

/**
 * @brief  Threads
 */
typedef struct
{
    uint32_t priority;   // thread priority
    uint32_t stack_size; // stack size
    const char *name;    // name
} config_thread_t;

/**
 * @brief   System critical signals
 */
typedef struct
{
    ADC_HandleTypeDef *hadc; // ADC handle
    uint8_t scan_slot;   // 0-based index into hadc's scan sequence (rank - 1)
    uint16_t min_adc;    // minimum expected ADC reading
    uint16_t max_adc;    // maximum expected ADC reading
    uint16_t min_mapped; // minimum mapped reading
    uint16_t max_mapped; // maximum mapped reading
    float outside_bounds_fraction; // fraction of mapped range defining out of bounds signal
} config_scs_t;

/**
 * @brief   Control
 */
typedef struct
{
    config_thread_t thread; // control thread config
    uint32_t schedule_ticks; // number of ticks between runs of the control loop thread
    bool r2d_requires_brake; // whether or not the brake needs to be pressed for R2D activation
    uint32_t ts_ready_timeout_ticks; // ticks after which waiting for TS ready times out
    uint32_t ts_ready_poll_ticks; // how often to poll input when waiting for TS ready
    uint32_t precharge_timeout_ticks; // ticks after which waiting for precharge times out
    uint32_t ready_wait_led_toggle_ticks; // ticks between toggling the TS on LED while waiting for 'TS ready' from relay controller
    uint32_t error_led_toggle_ticks; // ticks between toggling TS on LED in activation error
    uint16_t apps_bps_high_threshold; // apps reading to fault when brake also pressed
    uint16_t apps_bps_low_threshold; // apps reading to recover from fault
    uint16_t apps_bps_fault_bps_threshold; // live BPS reading threshold for apps/bps plausibility fault
    uint16_t bps_on_threshold;     // BPS reading to consider BPS 'on'
    uint16_t hard_max_torque;      // Hard maximum torque value (e.g. accel)
    uint16_t endurance_max_torque; // Max torque in endurance mode
    uint16_t crawl_max_torque;     // Max torque in crawl/reverse mode
} config_ctrl_t;

/**
 * @brief   Dash
 */
typedef struct
{
    config_thread_t thread; // dash thread config
    uint32_t btn_active_ticks; // ticks for which a button must be pressed for it to be considered 'activated'
    uint32_t btn_sample_ticks; // ticks between sampling buttons
    bool vc_run_check;         // whether or not the visual check should run
    uint32_t vc_led_on_ticks; // number of ticks for which the visual check should last
    bool vc_all_leds_on; // whether or not the visual check turns on all LEDs, or just the VC LEDs
    uint32_t vc_stagger_ticks; // ticks between turning on each visible check LED (set to zero to turn all on at once)
} config_dash_t;

/**
 * @brief   APPS
 */
typedef struct
{
    config_scs_t apps_1_scs;  // SCS configuration for first APPS signal
    config_scs_t apps_2_scs;  // SCS configuration for second APPS signal
    uint32_t max_discrepancy; // maximum discrepancy between APPS readings
    bool inverted;            // APPS% = 100 - calculated value
} config_apps_t;

/**
 * @brief   BPS
 */
typedef struct
{
    config_scs_t scs;             // SCS configuration
    float fully_pressed_fraction; // threshold above which considered 'fully pressed'
} config_bps_t;

/**
 * @brief   Ready to drive speaker
 */
typedef struct
{
    uint32_t active_ticks; // ticks for which RTDS sounds (one-shot R2D activation)
    uint32_t pulse_on_ticks;  // ticks RTDS is on during each reverse-mode pulse
    uint32_t pulse_off_ticks; // ticks RTDS is off between reverse-mode pulses
    GPIO_TypeDef *port;       // port for pin driving RTDS
    uint16_t pin;             // pin driving RTDS
} config_rtds_t;

/**
 * @brief   Torque map
 *
 * @note    All torque is represented as Nm * 10
 */
typedef struct
{
    torque_map_func_e function; // mapping function
    uint16_t input_max;      // maximum input value (range must be zero to max)
    uint16_t output_max;     // maximum output value (Nm * 10)
    float deadzone_fraction; // fraction of input range for deadzone
    float exponent; // curve exponent for TORQUE_MAP_EXPONENTIAL (input/input_max)^exponent
} config_torque_map_t;

/**
 * @brief   PM100DZ inverter
 */
typedef struct
{
    config_thread_t thread;           // service thread config
    uint32_t broadcast_timeout_ticks; // maximum number of ticks to wait for a broadcast
    uint32_t torque_request_timeout_ticks;
    uint32_t precharge_timeout_ticks;
    uint8_t speed_mode;
} config_pm100_t;

/**
 * @brief   CAN broadcasting service
 */
typedef struct
{
    config_thread_t thread;          // CANBC thread config
    uint32_t broadcast_period_ticks; // ticks between broadcasts
} config_canbc_t;

typedef struct
{
    config_thread_t thread;           // thread config
    uint32_t blink_period_ticks;      // period to blink the LED
    uint32_t fast_blink_period_ticks; // period to blink in fast mode
} config_heartbeat_t;

typedef struct
{
    config_thread_t thread;
    uint16_t period;
    uint32_t bps_light_threshold; // BPS (% * 10) threshold
    uint32_t bps_active_ticks; // time BPS must stay above threshold before treating as high
} config_tick_t;

typedef struct
{
    config_thread_t thread;
    uint16_t period;
    uint32_t broadcast_timeout_ticks;
    uint16_t torque_limit; // Nm, limit torque as a precaution
    uint16_t power_limit;  // W, limit power as a precaution
} config_remote_ctrl_t;

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
    config_log_level_t min_sd_log_level;
    UART_HandleTypeDef *uart;
    USART_HandleTypeDef *usart;
} config_log_t;

typedef struct
{
    uint8_t rtcan_s_priority;
    uint8_t rtcan_t_priority;
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

typedef struct
{
    config_thread_t thread;
    uint8_t ticks_per_wheel;
    float wheel_circumference_meters;
    uint32_t sample_period_ticks;
} config_wheelspeed_t;

/**
 * @brief   Audio-compressor-style soft-knee PI controller (deramp + gentle integral gain)
 */
typedef struct
{
    float threshold; // knee - measured value below this is untouched
    float p_gain;    // proportional de-ramp strength above threshold
    float i_gain;    // integral gain (unused without integral_enabled)
    float integral_decay; // per-tick multiplicative leak, 0..1 (100Hz loop tick, no explicit dt)
    float integral_max; // hard ceiling on accumulated integral, Nm*10 units (same as derate/torque_request)
    bool integral_enabled; // explicit on/off switch, rather than zeroing i_gain
} config_compressor_t;

/**
 * @brief   Torque limiters applied after the torque map: slip-ratio-based
 *          traction/launch control, and inverter DC-bus electrical power
 *          de-ramp
 */
typedef struct
{
    float gear_ratio; // motor : wheel reduction ratio (e.g. 120 motor rpm -> 37.7 wheel rpm)
    float min_motor_speed_rpm; // motor rpm floor below which slip limiting holds off entirely
    float min_denominator_rpm; // baseline min wheel rpm for the slip-ratio denominator / front-speed
                               // reliability gate; raised at init to >= min_motor_speed_rpm /
                               // gear_ratio so slip control can never engage before front wheel
                               // speed is itself reliably measurable (see torque_limiters_init)
    config_compressor_t slip; // slip de-ramp tuning (threshold is in slip %, unaffected by units)
    config_compressor_t power; // power de-ramp tuning (threshold in Watts, DC-bus)
} config_torque_limiters_t;

typedef struct
{
    config_thread_t thread;
    bool enable;
    uint32_t flush_period_ticks;
} config_sd_t;

typedef struct
{
    config_thread_t thread;
} config_usb_msc_t;

typedef struct
{
    config_scs_t sagl;        // ADC configuration for the steering angle signal
    config_scs_t mode_switch; // ADC configuration for the dash mode selector switch
    config_scs_t current;     // ADC configuration for the current sensor
} config_ext_inputs_t;

typedef struct
{
    config_thread_t thread;
    uint32_t broadcast_timeout_ticks; // maximum number of ticks to wait for a broadcast
    bool enable;
    bool inverted;
    uint16_t fan_on_threshold;  // temperature at which to turn on the fan
    uint16_t fan_off_threshold; // temperature at which to turn off the fan
} config_fans_t;

/**
 * @brief   VCU configuration
 *
 * @details The intended usage is that the main VCU module loads an instance of
 *          this struct and initialises all other modules based on its state.
 *          Theoretically all the individual modules could load the config,
 *          but that makes them less portable / decoupled.
 */
typedef struct
{
    config_dash_t dash;
    config_apps_t apps;
    config_bps_t bps;
    config_ctrl_t ctrl;
    config_rtds_t rtds;
    config_torque_map_t torque_map;
    config_pm100_t pm100;
    config_tick_t tick;
    config_remote_ctrl_t remote_ctrl;
    config_canbc_t canbc;
    config_heartbeat_t heartbeat;
    config_log_t log;
    config_rtos_t rtos;
    config_testbenches testbenches;
    config_wheelspeed_t wheelspeed;
    config_torque_limiters_t torque_limiters;
    config_sd_t sd;
    config_usb_msc_t usb_msc;
    config_ext_inputs_t ext_inputs;
    config_fans_t fans;
} config_t;

/*
 * public functions
 */
const config_t *config_get();


/***************************************************************************
 *
 * NOTE: MIGRATION OF OLD CONFIG SYSTEM BELOW IN PROGRESS!
 *
 ***************************************************************************/

/***************************************************************************
 * RTOS
//  ***************************************************************************/

// #define RTCAN_S_PRIORITY                    3
// #define RTCAN_T_PRIORITY                    2   // critical systems more
// important than sensors #define TS_CTRL_THREAD_PRIORITY		        2

// #define TRACEX_ENABLE                       0
//        // enable TraceX logging

// #define DRIVER_CTRL_TICK_RATE               100 // times per second

/***************************************************************************
 * CAN / inverter
 ***************************************************************************/

#define SELECTED_DRIVER_PROFILE DRIVER_PROFILE_DEFAULT

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
