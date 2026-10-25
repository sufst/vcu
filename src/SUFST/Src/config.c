#include "config.h"

#include <FreeRTOS.h>

/**
 * @brief   Convert seconds to ticks
 *
 * @note    The maximum precision is determined by how many ticks there are per
 *          second. E.g. configTICK_RATE_HZ = 1000 gives down to 0.001s.
 *          Antyhing less than this rounds down to zero ticks.
 */
#define SECONDS_TO_TICKS(x) (configTICK_RATE_HZ * (x))

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
 *
 * @note    Thread priorities are FreeRTOS priorities: a HIGHER number means a
 *          HIGHER priority (the opposite of ThreadX). The values below keep
 *          the relative ordering of the old ThreadX priorities
 *          (FreeRTOS priority = 40 - ThreadX priority), and must be below
 *          configMAX_PRIORITIES (56).
 *
 * @note    Stack sizes are in bytes.
 */
static const config_t config_instance = {
    .dash = {
        .thread = {
            .name = "Dash",
            .priority = 36,
            .stack_size = 2048
        },
        .btn_active_ticks = SECONDS_TO_TICKS(0.5),
        .vc_run_check = true,     
        .vc_led_on_ticks = SECONDS_TO_TICKS(2)
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
        }
    },
    .ctrl = {
        .thread = {
            .name = "Control",
            .priority = 38,
            .stack_size = 2048
        },
        .schedule_ticks = SECONDS_TO_TICKS(0.01), // 100Hz control loop
        .r2d_requires_brake = true,
        .bps_on_threshold = 50, // brake threshold (% * 10) for entering R2D when r2d_requires_brake = true
        .apps_bps_fault_bps_threshold = 150, // brake threshold (% * 10) to trigger apps bps fault
	    .apps_bps_low_threshold = 50, // required APPS (% * 10) to go out of a APPS bps fault
	    .apps_bps_high_threshold = 2000, // TEMPORARY (testing, 25 Oct): 2000 = brake + throttle check off. Put back 300 before competition. Required APPS (% * 10) to go into a APPS bps fault - set this to a big number (e.g. 2000), instead of default 300 (30%) to temporarily disable for scrut hardware demos
        .precharge_timeout_ticks = SECONDS_TO_TICKS(3),
        .error_led_toggle_ticks = SECONDS_TO_TICKS(0.1),
        .hard_max_torque = 2500,
        .endurance_max_torque = 2300, // 230 Nm
        .crawl_max_torque = 200
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
        .deadzone_fraction = 0.25f, // 25%, every mode: the pedal doesn't fully return (rested at up to ~21%). Not rescaled
        .exponent = 1.7f
    },
    .pm100 = {
        .thread = {
            .name = "PM100",
            .priority = 37,
            .stack_size = 2048
        },
        .broadcast_timeout_ticks = SECONDS_TO_TICKS(10),
        .torque_request_timeout_ticks = SECONDS_TO_TICKS(0.1), // 100 ms
        .precharge_timeout_ticks = SECONDS_TO_TICKS(1)
    },
    .tick = {
        .thread = {
            .name = "TICK",
            .priority = 37,
            .stack_size = 2048
        },
        .period = SECONDS_TO_TICKS(0.01),
        .bps_light_threshold = 200, // Turn breaklight on at 2%
        .bps_active_ticks = SECONDS_TO_TICKS(0.3), // debounce: BPS must be above threshold for 0.3s

    },
    .remote_ctrl = {
        .thread = {
            .name = "REMOTE",
            .priority = 37,
            .stack_size = 2048
        },
        .period = SECONDS_TO_TICKS(0.01),
        .torque_limit = 2300,
        .broadcast_timeout_ticks = SECONDS_TO_TICKS(1)
    },
    .canbc = {
        .thread = {
            .name = "CANBC",
            .priority = 36,
            .stack_size = 2048
        },
        .broadcast_period_ticks = SECONDS_TO_TICKS(0.1)
    },
    .heartbeat = {
        .thread = {
            .name = "HEARTBEAT",
            .priority = 24,
            .stack_size = 2048
        },
        .blink_period_ticks = SECONDS_TO_TICKS(0.25),
        .fast_blink_period_ticks = SECONDS_TO_TICKS(0.05)
    },
    .log = {
        .thread = {
            .name = "LOG",
            .priority = 25,
            .stack_size = 2048,
        },
        .min_level = LOG_LEVEL_INFO,
        .min_sd_log_level = LOG_LEVEL_WARN,
        .uart = &huart8,
        .usart = &husart1,
    },
    .sd = {
        .thread = {
            .name = "SD",
            .priority = 24,
            .stack_size = 8192,
        },
        .enable = true,
        .flush_period_ticks = SECONDS_TO_TICKS(10)
    },
    .rtos = {
        .rtcan_s_priority = 37,
        .rtcan_t_priority = 38, // critical systems bus is more important than sensors
    },
    .wheelspeed = {
        .thread = {
            .name = "WHEELSPEED",
            .priority = 36,
            .stack_size = 2048,
        },
        .ticks_per_wheel = 48,
        .wheel_circumference_meters = 1.305,
        .sample_period_ticks = SECONDS_TO_TICKS(0.02) // min viable speed approx 100rpm
    },
    .launch_ctrl = {
        // NOT REVIEWED, NOT BENCH TESTED, NOT RUN ON THE CAR: the launch part
        // runs in shadow mode (computes and logs its limit, always passes the
        // driver request through) until it has been checked on the car
        .enable = true,
        .shadow_mode = false,

        .gear_ratio = 3.18f,             // motor : wheel reduction
        .driven_radius_m = 0.20f,        // 16 in tyre = 0.203 m unloaded, ~0.20 loaded
        .front_circumference_m = 1.305f, // same as .wheelspeed
        .front_pulses_per_rev = 48.0f,   // same as .wheelspeed
        .front_window_s = 0.02f,         // same as .wheelspeed sample period
        .mass_kg = 300.0f,               // 280-300 kg with driver: top end is the safe side
        .driveline_eff = 0.93f,
        .accel_max_mps2 = 13.0f,         // ~1.3 g
        .motor_rpm_sign = -1.0f,         // measured: PM100 reports forward as negative

        .stationary_mps = 0.3f,
        .trigger_pedal = 900,            // 90% pedal starts the launch
        .pedal_pressed = 250,            // 25% pedal counts as pressed (arming); matches .torque_map.deadzone_fraction
        .arm_press_window_s = 0.5f,      // pressed 0.5 s without flooring it -> disarm
        .abort_torque = 300,             // 30 Nm
        .exit_speed_mps = 20.0f,
        .timeout_s = 10.0f,

        .slip_ratio = 0.10f,             // 10%: dry tyres peak ~10-15%; less scrubbing than 12% (sim)
        .slip_min_mps = 0.5f,            // was 0.8: allowed 27% slip at 3 m/s
        .kp = 180.0f,                    // 18 Nm per m/s of excess slip speed (200 + raw motor speed kept a 25 Hz shake going on 25 Oct)
        .ki = 3000.0f,                   // SIM
        .ramp_up_max = 3000.0f,          // 300 Nm/s
        .ki_recover = 6000.0f,           // 2 x ki: back up faster after a wheelspin cut
        .recover_max = 15000.0f,         // 1500 Nm/s: refill after a cut, up to the grip memory knee
        .recover_frac = 0.85f,          // after a cut, refill fast to 85% of the torque the wheels broke loose at
        .ramp_min_s = 0.2f,            // cuts in the first 0.2 s (motor speed noise) don't end the initial ramp
        .speed_avg_n = 3,               // controller averages motor speed over 30 ms (with err_band: stops the ~25 Hz driveshaft shake seen on 25 Oct)
        .err_band_mps = 0.3f,           // slip hysteresis: wobbles within +-0.3 m/s don't trigger cuts or fast refills
        .torque_start = 900,             // 90 Nm: first torque command of a launch (and the cap while ARMED)
        .torque_floor = 400,             // 40 Nm
        .torque_max = 2300,              // planned 220-230 Nm cap (<= .ctrl.hard_max_torque)
        .est_gain = 0.5f,

        .front_stale_s = 0.1f,           // 5 missed 20 ms samples
        .front_ahead_mps = 4.5f,         // FL jitters up to ~2 pulse steps (2.7 m/s) above the rears in the 24 Oct log
        .motor_reverse_rpm = 100.0f,
        .fault_torque = 200,             // 20 Nm while FAULT: same as .ctrl.crawl_max_torque

        // power limit: applies in every mode, also in shadow mode
        .power_limit_w = 75000.0f,       // 75 kW -> start of torque cut
        .power_kp = 0.2f                 // -100 Nm at 80 kW
    },
    .usb_msc = {
        .thread = {
            .name = "USB_MSC",
            .priority = 24,
            .stack_size = 2048
        }
    },
    .ext_inputs = {
        .sagl = {
            .hadc = &hadc1,
            .scan_slot = 3, // ADC1 rank 4
            .min_adc = 0,
            .max_adc = 4096,
            .min_mapped = 0,
            .max_mapped = 4000,
            .outside_bounds_fraction = 0.05f
        },
        .current = {
            .hadc = &hadc1,
            .scan_slot = 2, // ADC1 rank 3
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
            .priority = 28,
            .stack_size = 2048,
        },
        .broadcast_timeout_ticks = SECONDS_TO_TICKS(10),
        .enable = true,
        .inverted = true,
        .fan_on_threshold = 50, // to be adjusted to the actual value
        .fan_off_threshold = 48 // to be adjusted to the actual value
    }
};

/**
 * @brief   Returns the VCU configuration instance
 */
const config_t *config_get(void)
{
    return &config_instance;
}
