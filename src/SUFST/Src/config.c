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
        .btn_active_ticks = SECONDS_TO_TICKS(0.5),
        .btn_sample_ticks = SECONDS_TO_TICKS(0.1),
        .vc_run_check = true,     
        .vc_all_leds_on = true,
        .vc_led_on_ticks = SECONDS_TO_TICKS(2),
        .vc_stagger_ticks = SECONDS_TO_TICKS(0.25)
    },
    .apps = {
	 /*.apps_1_scs = {
            .hadc = &hadc1,
            .min_adc = 0,
            .max_adc = 0xFFF,
            .min_mapped = 0,
            .max_mapped = 0xFFF,
            .outside_bounds_fraction = 0.02f
        },
        .apps_2_scs = {
            .hadc = &hadc2,
            .min_adc = 0,
            .max_adc = 0xFFF,
            .min_mapped = 0,
            .max_mapped = 0xFFF,
            .outside_bounds_fraction = 0.02f
        },
        .max_discrepancy = 100000,*/
	 
	 .apps_1_scs = {
            .hadc = &hadc1,
            .min_adc = 6,
            .max_adc = 192,
            .min_mapped = 0,
            .max_mapped = 100,
            .outside_bounds_fraction = 0.05f
        },
        .apps_2_scs = {
            .hadc = &hadc2,
            .min_adc = 130,
            .max_adc = 255,
            .min_mapped = 0,
            .max_mapped = 100,
            .outside_bounds_fraction = 0.05f
        },
        .max_discrepancy = 10,
	
    },
    .bps = {
        .scs = {
            .hadc = &hadc3,
            .min_adc = 0,
            .max_adc = 350,
            .min_mapped = 0,
            .max_mapped = 200,
            .outside_bounds_fraction = 0.05f
        },
        .fully_pressed_fraction = 0.4f
    },
    .ctrl = {
        .thread = {
            .name = "Control",
            .priority = 2,
            .stack_size = 1024
        },
        .schedule_ticks = SECONDS_TO_TICKS(0.01), // 100Hz control loop
        .r2d_requires_brake = true,
        .bps_on_threshold = 40,
	    .apps_bps_low_threshold = 28,
	    .apps_bps_high_threshold = 30,
        .fan_on_threshold = 60, // to be adjusted to the actual value
        .fan_off_threshold = 50, // to be adjusted to the actual value
        .ts_ready_poll_ticks = SECONDS_TO_TICKS(0.1),
        .ts_ready_timeout_ticks = SECONDS_TO_TICKS(5),
        .precharge_timeout_ticks = SECONDS_TO_TICKS(3),
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
        .input_max = 100,
        .output_max = 1580, // maximum output value (Nm * 10)
        .deadzone_fraction = 0.28f,
        .speed_min = 1050, // minimum Torque request at max speed (Nm *10)
        .speed_start = 2500, // speed to start limiting torque (rpm)
        .speed_end = 3000 // speed for max torque limiting (rpm)
    },
    .pm100 = {
        .thread = {
            .name = "PM100",
            .priority = 3,
            .stack_size = 1024
        },
        .broadcast_timeout_ticks = SECONDS_TO_TICKS(10),
        .torque_request_timeout_ticks = SECONDS_TO_TICKS(0.1), // 100 ms
        .precharge_timeout_ticks = SECONDS_TO_TICKS(1),
        .speed_mode = 0
    },
    .tick = {
        .thread = {
            .name = "TICK",
            .priority = 3,
            .stack_size = 1024
        },
        .period = SECONDS_TO_TICKS(0.01)
    },
    .remote_ctrl = {
        .thread = {
            .name = "REMOTE",
            .priority = 3,
            .stack_size = 1024
        },
        .period = SECONDS_TO_TICKS(0.01),
        .torque_limit = 2000,
        .power_limit = 10000,
        .broadcast_timeout_ticks = SECONDS_TO_TICKS(1)
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
        .uart = &huart1
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
