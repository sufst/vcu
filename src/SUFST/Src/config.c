#include "config.h"

/**
 * @brief   Convert seconds to ticks
 * 
 * @note    The maximum precision is determined by how many ticks there are per
 *          second. E.g. TX_TIMER_TICKS_PER_SECOND = 1000 gives down to 0.001s.
 *          Antyhing less than this rounds down to zero ticks.
 */
#define SECONDS_TO_TICKS(x)  (TX_TIMER_TICKS_PER_SECOND * x)

/**
 * @brief   VCU configuration instance
 * 
 * @details See definitions of parameters in config.h
 *
 * @note    This struct is const, i.e. read only! The config should not change
 *          at runtime.
 *
 * @warning Make sure every single state in the struct is given a value,
 *          otherwise the value will default to zero
 */
static const config_t config_instance = {
    .dash = {
        .thread = {
            .name = "Dash",
            .priority = 4,
            .stack_size = 1024
        },
        .btn_active_ticks = SECONDS_TO_TICKS(1),
        .btn_sample_ticks = SECONDS_TO_TICKS(0.1),
        .vc_run_check = true,     
        .vc_all_leds_on = true,
        .vc_led_on_ticks = SECONDS_TO_TICKS(2),
        .vc_stagger_ticks = SECONDS_TO_TICKS(0.25)
    },
    .apps = {
        .apps_1_scs = {
            .hadc = &hadc1,
            .min_adc = 0x600,
            .max_adc = 0x900,
            .min_mapped = 0,
            .max_mapped = 1000,
            .outside_bounds_fraction = 0.05f
        },
        .apps_2_scs = {
            .hadc = &hadc2,
            .min_adc = 0x600,
            .max_adc = 0x900,
            .min_mapped = 0,
            .max_mapped = 1000,
            .outside_bounds_fraction = 0.05f
        },
        .max_discrepancy = 100, // 10% of range
    },
    .bps = {
        .scs = {
            .hadc = &hadc3,
            .min_adc = 0x600,
            .max_adc = 0x900,
            .min_mapped = 0,
            .max_mapped = 1000,
            .outside_bounds_fraction = 0.05f
        },
        .fully_pressed_fraction = 0.8f
    },
    .ctrl = {
        .thread = {
            .name = "Control",
            .priority = 2,
            .stack_size = 1024
        },
        .schedule_ticks = SECONDS_TO_TICKS(0.01), // 100Hz control loop
        .r2d_requires_brake = false,
        .ts_ready_poll_ticks = SECONDS_TO_TICKS(0.1),
        .ts_ready_timeout_ticks = SECONDS_TO_TICKS(5),
        .precharge_timeout_ticks = SECONDS_TO_TICKS(5),
        .ready_wait_led_toggle_ticks = SECONDS_TO_TICKS(0.5),
        .error_led_toggle_ticks = SECONDS_TO_TICKS(0.1)
    },
    .rtds = {
        .active_ticks = SECONDS_TO_TICKS(2),
        .port = R2D_SIREN_GPIO_Port,
        .pin = R2D_SIREN_Pin
    },
    .torque_map = {
        .function = TORQUE_MAP_LINEAR,
        .input_max = 1000,
        .output_max = 230,
        .deadzone_fraction = 0.05f
    },
    .pm100 = {
        .thread = {
            .name = "PM100",
            .priority = 3,
            .stack_size = 1024
        },
        .broadcast_timeout_ticks = SECONDS_TO_TICKS(10),
        .torque_request_timeout_ms = 100,
        .speed_mode = 0
    },
    .canbc = {
        .thread = {
            .name = "CANBC",
            .priority = 4,
            .stack_size = 1024
        },
        .broadcast_period_ticks = SECONDS_TO_TICKS(0.1)
    },
    .heartbeat = {
        .thread = {
            .name = "HEARTBEAT",
            .priority = 10,
            .stack_size = 512
        },
        .blink_period_ticks = SECONDS_TO_TICKS(0.25)
    },
    .log = {
        .thread = {
            .name = "LOG",
            .priority = 15,
            .stack_size = 1024,
        },
        .min_level = LOG_LEVEL_DEBUG,
        .uart = &huart3
    },
    .rtos = {
        .rtcan_s_priority = 3,
        .rtcan_c_priority = 2,
        .ts_ctrl_thread_priority = 2,
        .tracex_enable = false,
        .driver_ctrl_tick_rate = 100
    },
    .testbenches = {
        .run_apps_testbench = false,
        .run_fault_state_testbench = false,
        .apps_testbench_laps = 1
    }
};

/**
 * @brief   Returns the VCU configuration instance
 */
const config_t* config_get()
{
    return &config_instance;
}