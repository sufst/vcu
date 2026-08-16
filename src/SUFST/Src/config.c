#include "config.h"

/**
 * @brief   Convert seconds to ticks
 *
 * @note    The maximum precision is determined by how many ticks there are per
 *          second. E.g. TX_TIMER_TICKS_PER_SECOND = 1000 gives down to 0.001s.
 *          Antyhing less than this rounds down to zero ticks.
 */
#define SECONDS_TO_TICKS(x) (TX_TIMER_TICKS_PER_SECOND * x)

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
	    // .apps_1_scs = {
        //     .hadc = &hadc1,
        //     .adc_channel = ADC_CHANNEL_8,
        //     .min_adc = 0,
        //     .max_adc = 0xFFF,
        //     .min_mapped = 0,
        //     .max_mapped = 0xFFF,
        //     .outside_bounds_fraction = 0.02f
        // },
        // .apps_2_scs = {
        //     .hadc = &hadc2,
        //     .adc_channel = ADC_CHANNEL_15,
        //     .min_adc = 0,
        //     .max_adc = 0xFFF,
        //     .min_mapped = 0,
        //     .max_mapped = 0xFFF,
        //     .outside_bounds_fraction = 0.02f
        // },
        // .max_discrepancy = 100000,
	 
	 .apps_1_scs = {
            .hadc = &hadc1,
            .scan_slot = 0, // ADC1 rank 1
            .min_adc = 98, //79 measured - 77 set - 136 previous
            .max_adc = 1635, // 1830 measured - 1850 set - 1405 previous
            .min_mapped = 0,
            .max_mapped = 1000,
            .outside_bounds_fraction = 0.10f
        },
        .apps_2_scs = {
            .hadc = &hadc2,
            .scan_slot = 0, // ADC2 rank 1
            .min_adc = 792, // 750 measured - 740 set - 786 previous
            .max_adc = 2416, //2660 measured - 2700 set - 2050 previous
            .min_mapped = 0, 
            .max_mapped = 1000,
            .outside_bounds_fraction = 0.10f
        }, 
        .max_discrepancy = 120, // TODO: reduce this (ideally 10, worse case 25-30) by doing a better calibration once this is finalised in the car
        .inverted = true
    },
    .bps = {
        .scs = {
            .hadc = &hadc1,
            .scan_slot = 1, // ADC1 rank 2
            .min_adc = 380,
            .max_adc = 800,
            .min_mapped = 0,
            .max_mapped = 1000,
            .outside_bounds_fraction = 0.5f
        },
        .fully_pressed_fraction = 0.25f
    },
    .ctrl = {
        .thread = {
            .name = "Control",
            .priority = 2,
            .stack_size = 1024
        },
        .schedule_ticks = SECONDS_TO_TICKS(0.01), // 100Hz control loop
        .r2d_requires_brake = true,
        .bps_on_threshold = 50,
	    .apps_bps_low_threshold = 50,
	    .apps_bps_high_threshold = 300, // set this to 2000 (i.e. 200%), instead of default 300 (30%) to temporarily disable for scrut hardware demos
        .fan_on_threshold = 60, // to be adjusted to the actual value
        .fan_off_threshold = 50, // to be adjusted to the actual value
        .ts_ready_poll_ticks = SECONDS_TO_TICKS(0.1),
        .ts_ready_timeout_ticks = SECONDS_TO_TICKS(5),
        .precharge_timeout_ticks = SECONDS_TO_TICKS(3),
        .ready_wait_led_toggle_ticks = SECONDS_TO_TICKS(0.5),
        .error_led_toggle_ticks = SECONDS_TO_TICKS(0.1),
        .hard_max_torque = 2500,
        .endurance_max_torque = 1100,
        .crawl_max_torque = 200,
        .torque_ctrl_max_slip_percent = 10
    },
    .rtds = {
        .active_ticks = SECONDS_TO_TICKS(2),
        .pulse_on_ticks = SECONDS_TO_TICKS(0.1),
        .pulse_off_ticks = SECONDS_TO_TICKS(0.9),
        .port = R2D_SIREN_GPIO_Port,
        .pin = R2D_SIREN_Pin
    },
    .torque_map = {
        .function = TORQUE_MAP_EXPONENTIAL,
        .input_max = 1000, // percent * 10 so 100%
        .output_max = 1000, // initial default before R2D; torque_map_set_output_max() is called with .ctrl.{hard,endurance,crawl}_max_torque when entering R2D depending on the current mode
        .deadzone_fraction = 0.05f,
        .exponent = 1.7f
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
        .period = SECONDS_TO_TICKS(0.01),
        .bps_threshold = 200, // Turn breaklight on at 2%
        .bps_active_ticks = SECONDS_TO_TICKS(0.3), // debounce: BPS must be above threshold for 0.3s

    },
    .remote_ctrl = {
        .thread = {
            .name = "REMOTE",
            .priority = 3,
            .stack_size = 1024
        },
        .period = SECONDS_TO_TICKS(0.01),
        .torque_limit = 2300,
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
        .blink_period_ticks = SECONDS_TO_TICKS(0.25),
        .fast_blink_period_ticks = SECONDS_TO_TICKS(0.05)
    },
    .log = {
        .thread = {
            .name = "LOG",
            .priority = 15,
            .stack_size = 1024,
        },
        .min_level = LOG_LEVEL_INFO,
        .min_sd_log_level = LOG_LEVEL_WARN,
        .uart = &huart8,
        .usart = &husart1,
    },
    .sd = {
        .thread = {
            .name = "SD",
            .priority = 16,
            .stack_size = 8192,
        },
        .enable = true,
        .flush_period_ticks = SECONDS_TO_TICKS(10)
    },
    .rtos = {
        .rtcan_s_priority = 3,
        .rtcan_t_priority = 2,
        .ts_ctrl_thread_priority = 2,
        .tracex_enable = false,
        .driver_ctrl_tick_rate = 100
    },
    .testbenches = {
        .run_apps_testbench = false,
        .run_fault_state_testbench = false,
        .apps_testbench_laps = 1
    },
    .wheelspeed = {
        .thread = {
            .name = "WHEELSPEED",
            .priority = 14,
            .stack_size = 1024,
        },
        .ticks_per_wheel = 48,
        .wheel_circumference_meters = 1.305,
        .sample_period_ticks = SECONDS_TO_TICKS(0.01)
    },
    .usb_msc = {
        .thread = {
            .name = "USB_MSC",
            .priority = 5,
            .stack_size = 8192
        }
    },
    .ext_inputs = {
        .sagl = {
            .hadc = &hadc1,
            .scan_slot = 2, // ADC1 rank 3
            .min_adc = 0,
            .max_adc = 4096,
            .min_mapped = 0,
            .max_mapped = 4000,
            .outside_bounds_fraction = 0.05f
        },
        .current = {
            .hadc = &hadc1,
            .scan_slot = 3, // ADC1 rank 4
            .min_adc = 0,
            .max_adc = 4096,
            .min_mapped = 0,
            .max_mapped = 4000,
            .outside_bounds_fraction = 0.05f
        },
        .mode_switch = {
            .hadc = &hadc1,
            .scan_slot = 4, // ADC1 rank 5
            .min_adc = 0,
            .max_adc = 3300,
            .min_mapped = 0,
            .max_mapped = 330,
            .outside_bounds_fraction = 0.05f
        }
    },
    .fans = {
        .thread = {
            .name = "FANS",
            .priority = 12,
            .stack_size = 1024,
        },
        .broadcast_timeout_ticks = SECONDS_TO_TICKS(10),
        .enable = true,
        .inverted = true
    }
};

/**
 * @brief   Returns the VCU configuration instance
 */
const config_t *config_get()
{
    return &config_instance;
}
