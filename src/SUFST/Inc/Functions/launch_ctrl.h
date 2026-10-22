/******************************************************************************
 * @file    launch_ctrl.h
 * @brief   Launch control for standing starts (accel event).
 *
 * @details A torque LIMITER with a state machine:
 *            OFF -> ARMED (launch mode selected, car stopped, driver holds
 *                          the brake for arm_hold_s)
 *                -> ACTIVE (brake off, pedal floored to trigger_pedal)
 *                -> DONE (exit speed / lift-off / brake / timeout)
 *            ARMED -> OFF (car rolls with the pedal released, or the pedal
 *                          is pressed without reaching trigger_pedal within
 *                          arm_press_window_s: the driver is just driving)
 *            any -> FAULT (bad sensor data) -> plain driver torque
 *          OFF, DONE and FAULT all need the same stop + brake hold to arm
 *          again, so a launch never starts without the driver asking for it.
 *
 *          While ACTIVE it holds the driven wheels at a target slip:
 *            target wheel speed = max(v_ground * (1 + slip_ratio),
 *                                     v_ground + slip_min)
 *          using a PI controller whose integrator starts at a feedforward
 *          torque (torque_start). Ground speed comes from an estimator that
 *          predicts with the applied torque and is kept inside the band the
 *          coarse front wheel speed count allows. This is what lets it work
 *          from 0 m/s, where the front wheels still read 0.
 *
 *          After the launch part it applies the DC-bus power limit: above
 *          power_limit_w the torque is cut by power_kp per watt (all the way
 *          to 0 if the power reading is missing). The power limit applies in
 *          every drive mode, also with enable = false and in shadow mode.
 *
 *          Rules this module keeps:
 *            - output <= driver request, always (it never adds torque)
 *            - any doubt about the inputs -> FAULT -> driver torque
 *            - plain C, no RTOS calls, no globals: testable on a laptop and
 *              unaffected by a ThreadX -> FreeRTOS move
 *
 *          Torque units are Nm x 10, as in the rest of the VCU.
 *
 *          STATUS: NOT REVIEWED, NOT BENCH TESTED, NOT RUN ON THE CAR.
 *****************************************************************************/

#ifndef LAUNCH_CTRL_H
#define LAUNCH_CTRL_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    LAUNCH_CTRL_STATE_OFF = 0,
    LAUNCH_CTRL_STATE_ARMED = 1,
    LAUNCH_CTRL_STATE_ACTIVE = 2,
    LAUNCH_CTRL_STATE_DONE = 3,
    LAUNCH_CTRL_STATE_FAULT = 4
} launch_ctrl_state_t;

typedef enum
{
    LAUNCH_CTRL_FAULT_NONE = 0,
    LAUNCH_CTRL_FAULT_FRONT_STALE = 1,    // no new front wheel sample for too long
    LAUNCH_CTRL_FAULT_FRONT_DISAGREE = 2, // FL and FR disagree strongly at speed
    LAUNCH_CTRL_FAULT_MOTOR_REVERSE = 3,  // motor rpm negative (sign wrong?)
    LAUNCH_CTRL_FAULT_FRONT_AHEAD = 4,    // fronts much faster than driven wheels
    LAUNCH_CTRL_FAULT_BAD_DT = 5          // nonsense time step
} launch_ctrl_fault_t;

/**
 * @brief   Configuration. config.h includes this header and config_t holds
 *          it as .launch_ctrl; the values are in config.c.
 */
typedef struct
{
    bool enable;      // launch part on/off (the power limit always applies)
    bool shadow_mode; // launch part computes and logs everything, but passes driver torque on

    // vehicle
    float gear_ratio;            // motor : wheel, 3.18
    float driven_radius_m;       // rear rolling radius, 0.20 (measure!)
    float front_circumference_m; // 1.305 (config.c .wheelspeed)
    float front_pulses_per_rev;  // 48    (config.c .wheelspeed)
    float front_window_s;        // 0.02  (config.c .wheelspeed)
    float mass_kg;               // car + driver, for the speed prediction
    float driveline_eff;         // 0.93
    float accel_max_mps2;        // most acceleration the car can plausibly reach
    float motor_rpm_sign;        // +1, or -1 if the PM100 reports forward as negative

    // arming / exit
    float stationary_mps;      // below this the car counts as stationary
    float arm_hold_s;          // brake held this long at a standstill -> ARMED
    uint16_t trigger_pedal;    // pedal (% x 10) that starts a launch when ARMED
    uint16_t pedal_pressed;    // pedal (% x 10) above this counts as pressed
    float arm_press_window_s;  // pressed this long without reaching trigger_pedal -> disarm
    uint16_t abort_torque;   // request below this ends the launch (lift-off)
    float exit_speed_mps;    // hand back to normal driving above this
    float timeout_s;         // hand back after this long regardless

    // control
    float slip_ratio;      // target slip ratio, e.g. 0.12
    float slip_min_mps;    // minimum target slip speed at low speed
    float kp;              // Nm x 10 per (m/s) of slip-speed error
    float ki;              // Nm x 10 per (m/s x s)
    float ramp_up_max;     // max rise of the integrator, Nm x 10 per second
    uint16_t torque_start; // feedforward: integrator value at launch
    uint16_t torque_floor; // limit never goes below this while active
    uint16_t torque_max;   // integrator ceiling (normally the mode's max torque)
    float est_gain;        // 0..1, how hard a new front sample pulls the estimate

    // plausibility
    float front_stale_s;     // fault if no new front sample for this long
    float disagree_ratio;    // fault if |FL-FR|/max > this ...
    float disagree_min_mps;  // ... while the faster front is above this speed
    float front_ahead_mps;   // fault if front speed > driven speed + this
    float motor_reverse_rpm; // fault if motor rpm (after sign) below -this

    // power limit (every mode)
    float power_limit_w; // DC-bus power above which torque is cut (W)
    float power_kp;      // Nm x 10 cut per W above power_limit_w
} config_launch_ctrl_t;

typedef struct
{
    float dt_s;                // time since last call (control period, 0.01)
    uint16_t driver_torque;    // driver request after the torque map, Nm x 10
    int16_t motor_rpm;         // PM100 motor speed
    float fl_rpm;              // front left wheel rpm (wheelspeed.c)
    float fr_rpm;              // front right wheel rpm
    bool front_new_sample;     // true on the first call after a new 20 ms count
    uint16_t pedal;            // APPS reading, % x 10
    bool launch_mode_selected; // current_mode == CTRL_MODE_LAUNCH_CTRL
    bool brake_pressed;        // bps above the brake-on threshold
    float electrical_power_w;  // PM100 DC-bus power (W), INFINITY if unknown
} launch_ctrl_inputs_t;

typedef struct
{
    const config_launch_ctrl_t *cfg;
    launch_ctrl_state_t state;
    launch_ctrl_fault_t fault;

    float t_active_s;
    float brake_hold_s; // how long the brake has been held at a standstill (for arming)
    float press_s;      // how long the pedal has been pressed while ARMED
    bool stationary;    // car counts as stopped (for the dash)
    float since_front_s;
    float v_ground_est; // m/s
    float v_driven;     // m/s, from motor rpm
    float v_front;      // m/s, faster front wheel (as measured)
    float v_target;     // m/s, target driven-wheel speed
    float integrator;   // Nm x 10
    float limit;        // Nm x 10, what the launch part wants (logged even in shadow mode)
    uint16_t output;    // Nm x 10, what was returned (after the power limit)
} launch_ctrl_context_t;

void launch_ctrl_init(launch_ctrl_context_t *ctx, const config_launch_ctrl_t *cfg);

/**
 * @brief   Call once per control loop, every loop, including when the car is
 *          not moving and when the driver request is zero.
 * @return  Torque request (Nm x 10) to pass on. Always <= in->driver_torque.
 */
uint16_t launch_ctrl_apply(launch_ctrl_context_t *ctx, const launch_ctrl_inputs_t *in);

/** @brief True while the launch part is limiting (false in shadow mode). */
bool launch_ctrl_is_active(const launch_ctrl_context_t *ctx);

/**
 * @brief   Arming progress for the dash, 0..100 %: how far through the brake
 *          hold the driver is, 100 once ARMED or ACTIVE.
 */
uint8_t launch_ctrl_arm_progress(const launch_ctrl_context_t *ctx);

#endif
