/******************************************************************************
 * @file    launch_ctrl_config_default.c
 * @brief   Starting values for config_launch_ctrl_t, used by the laptop tests
 *          and the simulation. The car's real values are the
 *          `.launch_ctrl = { ... }` block in src/SUFST/Src/config.c.
 *
 *          Values marked SIM were tuned in launch_sim_v0.py only, which is
 *          tuned on itself. Values marked MEASURE must be checked on the car.
 *****************************************************************************/

#include "launch_ctrl.h"

const config_launch_ctrl_t launch_ctrl_config_default = {
    .enable = true,
    .shadow_mode = false,

    .gear_ratio = 3.18f,
    .driven_radius_m = 0.20f,        // 16 in tyre = 0.203 m unloaded, ~0.20 loaded
    .front_circumference_m = 1.305f, // MEASURE: roll-out test
    .front_pulses_per_rev = 48.0f,   // MEASURE: roll-out test
    .front_window_s = 0.02f,
    .mass_kg = 300.0f,               // 280-300 kg with driver: top end is the safe side
    .driveline_eff = 0.93f,
    .accel_max_mps2 = 13.0f,         // ~1.3 g
    .motor_rpm_sign = -1.0f,         // measured: PM100 reports forward as negative

    .stationary_mps = 0.3f,
    .trigger_pedal = 900,      // 90%
    .pedal_pressed = 50,       // 5%
    .arm_press_window_s = 0.5f,
    .abort_torque = 300,   // 30 Nm
    .exit_speed_mps = 20.0f,
    .timeout_s = 10.0f,    // was 6 s; raised so a wet launch doesn't hand back full torque mid-run

    .slip_ratio = 0.12f,    // SIM
    .slip_min_mps = 0.8f,   // SIM
    .kp = 200.0f,           // 20 Nm per m/s of excess slip speed
    .ki = 3000.0f,          // SIM
    .ramp_up_max = 3000.0f, // 300 Nm/s
    .ki_recover = 6000.0f,
    .recover_max = 15000.0f, // 1500 Nm/s
    .recover_frac = 0.85f,          // after a cut, refill fast to 85% of the torque the wheels broke loose at
    .ramp_min_s = 0.2f,            // cuts in the first 0.2 s (motor speed noise) don't end the initial ramp
    .torque_start = 1000,   // 100 Nm
    .torque_floor = 400,    // 40 Nm
    .torque_max = 2300,     // matches the planned 220-230 Nm cap
    .est_gain = 0.5f,

    .front_stale_s = 0.1f,  // 5 missed 20 ms samples
    .front_ahead_mps = 3.0f,
    .motor_reverse_rpm = 100.0f,
    .fault_torque = 200,  // 20 Nm (crawl torque) while FAULT

    .power_limit_w = 75000.0f, // 75 kW
    .power_kp = 0.2f,          // -100 Nm at 80 kW
};
