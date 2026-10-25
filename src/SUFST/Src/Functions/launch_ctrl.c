/******************************************************************************
 * @file    launch_ctrl.c
 * @brief   Launch control for standing starts, and the DC-bus power limit.
 *          See launch_ctrl.h.
 *
 *          STATUS: NOT REVIEWED, NOT BENCH TESTED, NOT RUN ON THE CAR.
 *****************************************************************************/

#include "launch_ctrl.h"

#include <stddef.h>

#define LC_PI 3.14159265f

static float clampf(float x, float lo, float hi)
{
    if (x < lo)
        return lo;
    if (x > hi)
        return hi;
    return x;
}

static float maxf(float a, float b) { return (a > b) ? a : b; }
static float minf(float a, float b) { return (a < b) ? a : b; }
static float absf(float a) { return (a < 0.0f) ? -a : a; }

/* m/s represented by one pulse in one counting window (1.36 m/s with 48 / 1.305 m / 20 ms) */
static float front_step_mps(const config_launch_ctrl_t *c)
{
    return c->front_circumference_m / (c->front_pulses_per_rev * c->front_window_s);
}

/* Front speed from FL only: the FR sensor picks up noise when the inverter
 * switches on. FL does too (logged: 0 -> 6.8 m/s in one 20 ms window, and
 * 0 for one window while moving), so:
 *  - while the motor is stopped the car is stopped (the rears are geared to
 *    the motor): the front speed is 0
 *  - otherwise a reading is only taken if it is within what the car could do
 *    since the last good reading: one pulse step of rounding plus accel_max
 *    for the time since then. A jump outside that keeps the last good value.
 *    A wrong reading that stays wrong is taken once the allowance has grown
 *    to it, so a stuck sensor still ends up in FRONT_AHEAD. */
static void update_front(launch_ctrl_context_t *ctx, const launch_ctrl_inputs_t *in)
{
    const config_launch_ctrl_t *c = ctx->cfg;
    float raw = in->fl_rpm / 60.0f * c->front_circumference_m;

    ctx->front_age_s += in->dt_s;
    float allowed = front_step_mps(c) + c->accel_max_mps2 * ctx->front_age_s;

    if (absf(ctx->v_driven) < c->stationary_mps)
    {
        ctx->v_front = 0.0f;
        ctx->front_age_s = 0.0f;
    }
    else if (absf(raw - ctx->v_front) <= allowed)
    {
        ctx->v_front = raw;
        ctx->front_age_s = 0.0f;
    }
}

static bool is_stationary(const launch_ctrl_context_t *ctx)
{
    const config_launch_ctrl_t *c = ctx->cfg;
    return (absf(ctx->v_driven) < c->stationary_mps) &&
        (ctx->v_front < 0.5f * front_step_mps(c));
}

static launch_ctrl_fault_t check_inputs(const launch_ctrl_context_t *ctx,
                                        const launch_ctrl_inputs_t *in,
                                        float motor_rpm_signed)
{
    const config_launch_ctrl_t *c = ctx->cfg;

    if (!(in->dt_s > 0.0f) || in->dt_s > 0.1f)
        return LAUNCH_CTRL_FAULT_BAD_DT;

    if (ctx->since_front_s > c->front_stale_s)
        return LAUNCH_CTRL_FAULT_FRONT_STALE;

    if (motor_rpm_signed < -c->motor_reverse_rpm)
        return LAUNCH_CTRL_FAULT_MOTOR_REVERSE;

    if (ctx->v_front > ctx->v_driven + c->front_ahead_mps)
        return LAUNCH_CTRL_FAULT_FRONT_AHEAD;

    return LAUNCH_CTRL_FAULT_NONE;
}

/* Ground speed: predict from the torque the car actually got, then keep the
 * estimate inside what the latest front pulse count allows. */
static void update_estimate(launch_ctrl_context_t *ctx, const launch_ctrl_inputs_t *in)
{
    const config_launch_ctrl_t *c = ctx->cfg;

    float torque_nm = (float)ctx->output / 10.0f;
    float a_pred = torque_nm * c->gear_ratio * c->driveline_eff / (c->driven_radius_m * c->mass_kg);
    a_pred = clampf(a_pred, 0.0f, c->accel_max_mps2);
    ctx->v_ground_est += a_pred * in->dt_s;

    /* The count rounds DOWN: a reading of n pulses means the average speed over
     * the window was between n and n+1 pulses' worth. So the best single guess
     * is the middle of that range, not the reading itself. A reading of 0 only
     * says "slower than one pulse per window". */
    float q = front_step_mps(c);
    if (in->front_new_sample)
    {
        float meas = (ctx->v_front > 0.0f) ? ctx->v_front + 0.5f * q : minf(ctx->v_ground_est, 0.5f * q);
        ctx->v_ground_est += c->est_gain * (meas - ctx->v_ground_est);
    }

    /* Hard band from the same reasoning. During a launch the car only speeds
     * up, so the speed now is at least the window average, and at most one
     * pulse plus one window of acceleration above the reading. */
    float lo = ctx->v_front;
    float hi = ctx->v_front + q + c->accel_max_mps2 * c->front_window_s;
    ctx->v_ground_est = clampf(ctx->v_ground_est, lo, hi);

    /* under drive the car cannot be faster than its driven wheels */
    if (ctx->v_driven >= 0.0f)
    {
        ctx->v_ground_est = minf(ctx->v_ground_est, ctx->v_driven);
    }
}

static float control_step(launch_ctrl_context_t *ctx, const launch_ctrl_inputs_t *in)
{
    const config_launch_ctrl_t *c = ctx->cfg;

    ctx->v_target = maxf(ctx->v_ground_est * (1.0f + c->slip_ratio),
                         ctx->v_ground_est + c->slip_min_mps);
    float err = ctx->v_driven - ctx->v_target; // > 0: too much wheelspin

    /* grip memory: the limit at the moment the wheels broke loose */
    if (err > 0.0f && ctx->prev_err <= 0.0f && ctx->ramp_ref >= 0.0f)
    {
        ctx->slip_torque = ctx->ramp_ref;
    }
    ctx->prev_err = err;

    /* after the first cut, while the wheels grip: refill fast (recover_max) to
     * just under the torque they broke loose at, then only creep above it
     * (ramp_up_max) to look for more grip. Snapping straight back to the full
     * request just spins them up again */
    float d;
    if (!ctx->ramping && err < 0.0f)
    {
        float knee = c->recover_frac * ctx->slip_torque;
        if (ctx->integrator < knee)
        {
            d = minf(c->recover_max * in->dt_s, knee - ctx->integrator);
        }
        else
        {
            d = minf(-c->ki_recover * err * in->dt_s, c->ramp_up_max * in->dt_s);
        }
    }
    else
    {
        d = minf(-c->ki * err * in->dt_s, c->ramp_up_max * in->dt_s);
    }
    ctx->integrator += d;

    /* anti-windup: never sit above what the driver is asking for */
    float upper = maxf((float)c->torque_floor, minf((float)c->torque_max, (float)in->driver_torque));
    ctx->integrator = clampf(ctx->integrator, (float)c->torque_floor, upper);

    return clampf(ctx->integrator - c->kp * err, (float)c->torque_floor, (float)c->torque_max);
}

void launch_ctrl_init(launch_ctrl_context_t *ctx, const config_launch_ctrl_t *cfg)
{
    ctx->cfg = cfg;
    ctx->state = LAUNCH_CTRL_STATE_OFF;
    ctx->fault = LAUNCH_CTRL_FAULT_NONE;
    ctx->t_active_s = 0.0f;
    ctx->press_s = 0.0f;
    ctx->stationary = false;
    ctx->since_front_s = 0.0f;
    ctx->v_ground_est = 0.0f;
    ctx->v_driven = 0.0f;
    ctx->v_front = 0.0f;
    ctx->front_age_s = 0.0f;
    ctx->v_target = 0.0f;
    ctx->integrator = 0.0f;
    ctx->ramp_ref = -1.0f;
    ctx->ramping = false;
    ctx->slip_torque = 0.0f;
    ctx->prev_err = 0.0f;
    ctx->limit = 0.0f;
    ctx->output = 0;
}

/* DC-bus power limit: cut power_kp per watt above the limit, all the way to 0
 * if the reading is unknown (INFINITY / NaN) */
static uint16_t power_limit(const config_launch_ctrl_t *c, uint16_t torque, float power_w)
{
    if (power_w <= c->power_limit_w)
        return torque;
    float derate = (power_w - c->power_limit_w) * c->power_kp;
    if (!(derate < (float)torque))
        return 0;
    return (uint16_t)((float)torque - derate);
}

static uint16_t launch_step(launch_ctrl_context_t *ctx, const launch_ctrl_inputs_t *in)
{
    const config_launch_ctrl_t *c = ctx->cfg;
    uint16_t driver = in->driver_torque;

    /* measurements */
    float motor_rpm_signed = c->motor_rpm_sign * (float)in->motor_rpm;
    ctx->v_driven = motor_rpm_signed / c->gear_ratio * (2.0f * LC_PI / 60.0f) * c->driven_radius_m;
    update_front(ctx, in);
    ctx->since_front_s = in->front_new_sample ? 0.0f : ctx->since_front_s + in->dt_s;
    ctx->limit = (float)driver;
    ctx->stationary = is_stationary(ctx);

    if (!c->enable || !in->launch_mode_selected)
    {
        ctx->state = LAUNCH_CTRL_STATE_OFF;
        ctx->fault = LAUNCH_CTRL_FAULT_NONE;
            return driver;
    }

    bool stationary = ctx->stationary;
    bool pressed = in->pedal > c->pedal_pressed;

    /* plausibility only matters when we might act on the data */
    if (ctx->state == LAUNCH_CTRL_STATE_ARMED || ctx->state == LAUNCH_CTRL_STATE_ACTIVE)
    {
        launch_ctrl_fault_t f = check_inputs(ctx, in, motor_rpm_signed);
        if (f != LAUNCH_CTRL_FAULT_NONE)
        {
            ctx->state = LAUNCH_CTRL_STATE_FAULT;
            ctx->fault = f;
        }
    }

    switch (ctx->state)
    {
    case LAUNCH_CTRL_STATE_OFF:
    case LAUNCH_CTRL_STATE_DONE:
    case LAUNCH_CTRL_STATE_FAULT:
        /* arm by itself in launch mode: car stopped with the pedal released.
         * After a fault only once the inputs look right again, so a fault
         * that persists can't flip FAULT <-> ARMED every step */
        if (stationary && !pressed && check_inputs(ctx, in, motor_rpm_signed) == LAUNCH_CTRL_FAULT_NONE)
        {
            ctx->state = LAUNCH_CTRL_STATE_ARMED;
            ctx->fault = LAUNCH_CTRL_FAULT_NONE;
            ctx->press_s = 0.0f;
        }
        break;

    case LAUNCH_CTRL_STATE_ARMED:
        ctx->press_s = pressed ? ctx->press_s + in->dt_s : 0.0f;
        if (in->pedal >= c->trigger_pedal && !in->brake_pressed)
        {
            ctx->state = LAUNCH_CTRL_STATE_ACTIVE;
            ctx->t_active_s = 0.0f;
            ctx->v_ground_est = 0.0f;
            ctx->integrator = (float)c->torque_start;
            ctx->ramp_ref = -1.0f; // first step: limit = torque_start
            ctx->ramping = true;
            ctx->slip_torque = (float)c->torque_max;
            ctx->prev_err = 0.0f;
        }
        else if (ctx->press_s > c->arm_press_window_s)
        {
            /* just driving: disarm. Rolling with the pedal released does
             * not disarm, so a launch can start on a slope */
            ctx->state = LAUNCH_CTRL_STATE_OFF;
        }
        break;

    case LAUNCH_CTRL_STATE_ACTIVE:
        ctx->t_active_s += in->dt_s;
        if (driver < c->abort_torque || in->brake_pressed ||
            ctx->t_active_s > c->timeout_s || ctx->v_ground_est > c->exit_speed_mps)
        {
            ctx->state = LAUNCH_CTRL_STATE_DONE;
        }
        break;
    }

    uint16_t out = driver;

    if (ctx->state == LAUNCH_CTRL_STATE_FAULT)
    {
        /* bad sensor data: limp at crawl torque rather than handing the
         * driver the full request with no wheelspin control */
        ctx->limit = (float)c->fault_torque;
        if (!c->shadow_mode && ctx->limit < (float)driver)
        {
            out = (uint16_t)ctx->limit;
        }
    }
    else if (ctx->state == LAUNCH_CTRL_STATE_ARMED)
    {
        /* the pedal on its way to trigger_pedal: no more than the launch's
         * first command (a part-pedal drive-off disarms after
         * arm_press_window_s and then gets the full request) */
        ctx->limit = (float)c->torque_start;
        if (!c->shadow_mode && ctx->limit < (float)driver)
        {
            out = (uint16_t)ctx->limit;
        }
    }
    else if (ctx->state == LAUNCH_CTRL_STATE_ACTIVE)
    {
        update_estimate(ctx, in);
        /* the first command is torque_start, then the limit rises by at most
         * ramp_up_max until the controller first asks for less than the
         * limit it already has (the first wheelspin cut). From then on the controller alone sets the limit,
         * so it comes back as soon as the wheels grip again */
        float want = control_step(ctx, in);
        if (ctx->ramping)
        {
            float cap = (ctx->ramp_ref < 0.0f) ? (float)c->torque_start :
                                                 ctx->ramp_ref + c->ramp_up_max * in->dt_s;
            if (ctx->ramp_ref >= 0.0f && want < ctx->ramp_ref && ctx->t_active_s >= c->ramp_min_s)
                ctx->ramping = false; // a real cut, not just a slower rise or early noise
            want = minf(want, cap);
        }
        ctx->limit = want;
        ctx->ramp_ref = ctx->limit;
        if (!c->shadow_mode && ctx->limit < (float)driver)
        {
            out = (uint16_t)ctx->limit;
        }
    }

    return out;
}

uint16_t launch_ctrl_apply(launch_ctrl_context_t *ctx, const launch_ctrl_inputs_t *in)
{
    uint16_t out = power_limit(ctx->cfg, launch_step(ctx, in), in->electrical_power_w);
    ctx->output = out; // the torque the car gets: the next speed prediction uses it
    return out;
}

bool launch_ctrl_is_active(const launch_ctrl_context_t *ctx)
{
    return ctx->state == LAUNCH_CTRL_STATE_ACTIVE && !ctx->cfg->shadow_mode;
}

uint8_t launch_ctrl_arm_progress(const launch_ctrl_context_t *ctx)
{
    /* arming is immediate now (no brake hold): 100% once armed */
    return (ctx->state == LAUNCH_CTRL_STATE_ARMED || ctx->state == LAUNCH_CTRL_STATE_ACTIVE) ? 100 : 0;
}
