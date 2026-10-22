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

    float fl = in->fl_rpm / 60.0f * c->front_circumference_m;
    float fr = in->fr_rpm / 60.0f * c->front_circumference_m;
    float fmax = maxf(fl, fr);
    /* allow one pulse step of rounding on top of the ratio, or two coarse
     * counts one pulse apart look like a big disagreement at low speed */
    if (fmax > c->disagree_min_mps &&
        absf(fl - fr) > c->disagree_ratio * fmax + front_step_mps(c))
        return LAUNCH_CTRL_FAULT_FRONT_DISAGREE;

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

    float d = -c->ki * err * in->dt_s;
    d = minf(d, c->ramp_up_max * in->dt_s);
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
    ctx->brake_hold_s = 0.0f;
    ctx->press_s = 0.0f;
    ctx->stationary = false;
    ctx->since_front_s = 0.0f;
    ctx->v_ground_est = 0.0f;
    ctx->v_driven = 0.0f;
    ctx->v_front = 0.0f;
    ctx->v_target = 0.0f;
    ctx->integrator = 0.0f;
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
    ctx->v_front = maxf(in->fl_rpm, in->fr_rpm) / 60.0f * c->front_circumference_m;
    ctx->since_front_s = in->front_new_sample ? 0.0f : ctx->since_front_s + in->dt_s;
    ctx->limit = (float)driver;
    ctx->stationary = is_stationary(ctx);

    if (!c->enable || !in->launch_mode_selected)
    {
        ctx->state = LAUNCH_CTRL_STATE_OFF;
        ctx->fault = LAUNCH_CTRL_FAULT_NONE;
        ctx->brake_hold_s = 0.0f;
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
        /* arm only when the driver asks for it: stopped, brake held for
         * arm_hold_s without a break */
        if (stationary && in->brake_pressed)
        {
            ctx->brake_hold_s += in->dt_s;
            if (ctx->brake_hold_s >= c->arm_hold_s)
            {
                ctx->state = LAUNCH_CTRL_STATE_ARMED;
                ctx->fault = LAUNCH_CTRL_FAULT_NONE;
                ctx->brake_hold_s = 0.0f;
                ctx->press_s = 0.0f;
            }
        }
        else
        {
            ctx->brake_hold_s = 0.0f;
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
        }
        else if ((!stationary && !pressed) || ctx->press_s > c->arm_press_window_s)
        {
            ctx->state = LAUNCH_CTRL_STATE_OFF; // rolled or just driving: disarm
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

    if (ctx->state == LAUNCH_CTRL_STATE_ACTIVE)
    {
        update_estimate(ctx, in);
        ctx->limit = control_step(ctx, in);
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
    if (ctx->state == LAUNCH_CTRL_STATE_ARMED || ctx->state == LAUNCH_CTRL_STATE_ACTIVE)
        return 100;
    if (ctx->cfg == NULL || !(ctx->cfg->arm_hold_s > 0.0f)) // NULL: ctrl init failed early
        return 0;
    return (uint8_t)clampf(100.0f * ctx->brake_hold_s / ctx->cfg->arm_hold_s, 0.0f, 100.0f);
}
