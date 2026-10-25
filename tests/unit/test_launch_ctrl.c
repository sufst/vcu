/******************************************************************************
 * @file    test_launch_ctrl.c
 * @brief   Laptop unit tests for launch_ctrl.c, with the starting values in
 *          launch_ctrl_config_default.c (not config.c). Built and run by
 *          tests/run_tests.sh.
 *          These check the safety properties, not lap time (that is the sim).
 *****************************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "launch_ctrl.h"

extern const config_launch_ctrl_t launch_ctrl_config_default;

static int checks = 0, failures = 0;
#define CHECK(cond, msg)                                                       \
    do                                                                         \
    {                                                                          \
        checks++;                                                              \
        if (!(cond))                                                           \
        {                                                                      \
            failures++;                                                        \
            printf("  [FAIL] %s (line %d)\n", msg, __LINE__);                  \
        }                                                                      \
    } while (0)

/* PM100 motor rpm reading that makes the driven wheels go at v m/s (with the
 * PM100's sign convention: forward reads negative on our car) */
static int16_t rpm_for(float v)
{
    return (int16_t)lroundf(launch_ctrl_config_default.motor_rpm_sign * v / 0.20f /
                            (2.0f * 3.14159265f / 60.0f) * 3.18f);
}
/* wheel rpm reading for a front speed of v m/s, as the 48-pulse 20 ms count would report it */
static float front_rpm_for(float v)
{
    float pulses = floorf(v / 1.305f * 48.0f * 0.02f);
    return pulses / (48.0f * 0.02f) * 60.0f;
}

static launch_ctrl_inputs_t inputs(uint16_t drv, float v_driven, float v_front, int step)
{
    launch_ctrl_inputs_t in;
    memset(&in, 0, sizeof in);
    in.dt_s = 0.01f;
    in.driver_torque = drv;
    in.pedal = (uint16_t)((drv >= 2300) ? 1000 : drv * 1000 / 2300); // 2300 = floored
    in.motor_rpm = rpm_for(v_driven);
    in.fl_rpm = front_rpm_for(v_front);
    in.front_new_sample = (step % 2) == 0; // 20 ms window, 10 ms loop
    in.launch_mode_selected = true;
    in.brake_pressed = false;
    return in;
}

/* stand still with the pedal released for t seconds */
static void stand_still(launch_ctrl_context_t *ctx, float t)
{
    for (int i = 0; i < (int)lroundf(t / 0.01f); i++)
    {
        launch_ctrl_inputs_t in = inputs(0, 0.0f, 0.0f, i);
        launch_ctrl_apply(ctx, &in);
    }
}

/* launch mode arms by itself: just stop with the pedal released */
static void arm(launch_ctrl_context_t *ctx)
{
    stand_still(ctx, 0.05f);
}

static void test_disabled_passes_through(void)
{
    printf("disabled / mode not selected -> driver torque unchanged\n");
    config_launch_ctrl_t cfg = launch_ctrl_config_default;
    cfg.enable = false;
    launch_ctrl_context_t ctx;
    launch_ctrl_init(&ctx, &cfg);
    int ok = 1;
    for (int i = 0; i < 300; i++)
    {
        launch_ctrl_inputs_t in = inputs(2300, 10.0f, 0.0f, i);
        ok &= (launch_ctrl_apply(&ctx, &in) == 2300);
    }
    CHECK(ok, "enable=false must return the driver request");

    launch_ctrl_init(&ctx, &launch_ctrl_config_default);
    arm(&ctx);
    launch_ctrl_inputs_t in = inputs(2300, 10.0f, 0.0f, 0);
    in.launch_mode_selected = false;
    CHECK(launch_ctrl_apply(&ctx, &in) == 2300, "mode not selected must return the driver request");
    CHECK(ctx.state == LAUNCH_CTRL_STATE_OFF, "mode not selected -> OFF");
}

static void test_arms_only_when_stationary(void)
{
    printf("arms only from rest; a rolling start does not launch\n");
    launch_ctrl_context_t ctx;
    launch_ctrl_init(&ctx, &launch_ctrl_config_default);
    arm(&ctx);
    CHECK(ctx.state == LAUNCH_CTRL_STATE_ARMED, "stationary + pedal released -> ARMED");

    launch_ctrl_init(&ctx, &launch_ctrl_config_default);
    for (int i = 0; i < 50; i++)
    {
        launch_ctrl_inputs_t in = inputs(200, 5.0f, 5.0f, i); // already rolling
        launch_ctrl_apply(&ctx, &in);
    }
    CHECK(ctx.state == LAUNCH_CTRL_STATE_OFF, "rolling car must not arm");
    launch_ctrl_inputs_t in = inputs(2300, 8.0f, 5.0f, 0);
    CHECK(launch_ctrl_apply(&ctx, &in) == 2300, "rolling car gets driver torque");
}

static void test_limits_wheelspin_from_rest(void)
{
    printf("launch from rest with the rear wheels spinning -> torque cut\n");
    launch_ctrl_context_t ctx;
    launch_ctrl_init(&ctx, &launch_ctrl_config_default);
    arm(&ctx);
    uint16_t out = 0;
    for (int i = 0; i < 20; i++)
    {
        float v_wheels = 0.5f * (float)i; // rears spinning up fast, car barely moving
        launch_ctrl_inputs_t in = inputs(2300, v_wheels, 0.0f, i);
        out = launch_ctrl_apply(&ctx, &in);
    }
    CHECK(ctx.state == LAUNCH_CTRL_STATE_ACTIVE, "should be ACTIVE");
    CHECK(out < 2300, "spinning from rest must be limited (the old limiter is off here)");
    CHECK(out >= launch_ctrl_config_default.torque_floor, "never below the floor");
}

static void test_releases_when_gripping(void)
{
    printf("no wheelspin -> limit climbs back to the driver request\n");
    launch_ctrl_context_t ctx;
    launch_ctrl_init(&ctx, &launch_ctrl_config_default);
    arm(&ctx);
    uint16_t out = 0;
    float v = 0.0f;
    for (int i = 0; i < 150; i++)
    {
        v += 8.0f * 0.01f; // 0.8 g, wheels at 5% slip
        launch_ctrl_inputs_t in = inputs(2300, v * 1.05f + 0.1f, v, i);
        out = launch_ctrl_apply(&ctx, &in);
    }
    CHECK(out == 2300, "with low slip the driver should get full torque within 1.5 s");
}

static void test_never_exceeds_driver_fuzz(void)
{
    printf("fuzz: 200k random steps, output never above driver request\n");
    launch_ctrl_context_t ctx;
    launch_ctrl_init(&ctx, &launch_ctrl_config_default);
    srand(1);
    int ok = 1;
    for (int i = 0; i < 200000; i++)
    {
        if (i % 2000 == 0)
            arm(&ctx); // random inputs rarely stand still, so arm now and then
        launch_ctrl_inputs_t in;
        in.dt_s = (rand() % 50 == 0) ? 0.0f : 0.01f;
        in.driver_torque = (uint16_t)(rand() % 2600);
        in.pedal = (uint16_t)(rand() % 1001);
        in.motor_rpm = (int16_t)(launch_ctrl_config_default.motor_rpm_sign * (float)(rand() % 8000 - 500));
        in.fl_rpm = (float)(rand() % 1500);
        in.front_new_sample = rand() % 2;
        in.launch_mode_selected = rand() % 20 != 0;
        in.brake_pressed = rand() % 30 == 0;
        in.electrical_power_w = (rand() % 50 == 0) ? INFINITY : (float)(rand() % 90000);
        if (rand() % 100 == 0)
        {
            in.driver_torque = 0;
            in.pedal = 0;
            in.motor_rpm = 0;
            in.fl_rpm = 0;
        }
        uint16_t out = launch_ctrl_apply(&ctx, &in);
        ok &= (out <= in.driver_torque);
    }
    CHECK(ok, "output must never exceed the driver request");
}

static void test_lift_off_and_brake_end_launch(void)
{
    printf("lift-off or brake ends the launch\n");
    launch_ctrl_context_t ctx;
    launch_ctrl_init(&ctx, &launch_ctrl_config_default);
    arm(&ctx);
    for (int i = 0; i < 10; i++)
    {
        launch_ctrl_inputs_t in = inputs(2300, 3.0f, 0.0f, i);
        launch_ctrl_apply(&ctx, &in);
    }
    launch_ctrl_inputs_t in = inputs(100, 3.0f, 1.5f, 10);
    CHECK(launch_ctrl_apply(&ctx, &in) == 100, "lift-off returns driver torque");
    CHECK(ctx.state == LAUNCH_CTRL_STATE_DONE, "lift-off -> DONE");

    launch_ctrl_init(&ctx, &launch_ctrl_config_default);
    arm(&ctx);
    for (int i = 0; i < 10; i++)
    {
        launch_ctrl_inputs_t in2 = inputs(2300, 3.0f, 0.0f, i);
        launch_ctrl_apply(&ctx, &in2);
    }
    in = inputs(2300, 3.0f, 1.5f, 10);
    in.brake_pressed = true;
    launch_ctrl_apply(&ctx, &in);
    CHECK(ctx.state == LAUNCH_CTRL_STATE_DONE, "brake -> DONE");

    /* still moving: stays DONE; stopped with the pedal released: re-arms */
    for (int i = 0; i < 100; i++)
    {
        in = inputs(0, 2.0f, 1.5f, i);
        launch_ctrl_apply(&ctx, &in);
    }
    CHECK(ctx.state == LAUNCH_CTRL_STATE_DONE, "still moving after the launch -> stays DONE");
    arm(&ctx);
    CHECK(ctx.state == LAUNCH_CTRL_STATE_ARMED, "stopped again -> re-arms by itself");
}

static void run_to_active(launch_ctrl_context_t *ctx);

static void test_arming_protocol(void)
{
    printf("arming: launch mode + stopped + pedal released arms; full pedal starts the launch\n");
    const config_launch_ctrl_t *c = &launch_ctrl_config_default;
    launch_ctrl_context_t ctx;
    launch_ctrl_inputs_t in;

    launch_ctrl_init(&ctx, c);
    CHECK(launch_ctrl_arm_progress(&ctx) == 0, "not armed -> progress 0%");
    in = inputs(0, 0.0f, 0.0f, 0);
    launch_ctrl_apply(&ctx, &in);
    CHECK(ctx.state == LAUNCH_CTRL_STATE_ARMED, "stopped, pedal released -> ARMED straight away, no brake");
    CHECK(ctx.stationary, "stationary flag set while stopped");
    CHECK(launch_ctrl_arm_progress(&ctx) == 100, "ARMED -> progress 100%");

    launch_ctrl_init(&ctx, c);
    for (int i = 0; i < 100; i++)
    {
        in = inputs(300, 0.0f, 0.0f, i); // foot on the pedal while stopped
        launch_ctrl_apply(&ctx, &in);
    }
    CHECK(ctx.state == LAUNCH_CTRL_STATE_OFF, "pedal pressed while stopped -> does not arm");

    launch_ctrl_init(&ctx, c);
    for (int i = 0; i < 100; i++)
    {
        in = inputs(0, 1.0f, 0.0f, i); // creeping
        launch_ctrl_apply(&ctx, &in);
    }
    CHECK(ctx.state == LAUNCH_CTRL_STATE_OFF, "moving -> does not arm");

    launch_ctrl_init(&ctx, c);
    arm(&ctx);
    in = inputs(0, 1.0f, 0.0f, 0); // rolls with the pedal released (slope)
    launch_ctrl_apply(&ctx, &in);
    CHECK(ctx.state == LAUNCH_CTRL_STATE_ARMED, "armed car rolls -> stays armed");

    launch_ctrl_init(&ctx, c);
    arm(&ctx);
    uint16_t out = 0;
    for (int i = 0; i < 60; i++)
    {
        in = inputs(700, 0.5f, 0.0f, i); // ~30% pedal, just driving off
        out = launch_ctrl_apply(&ctx, &in);
    }
    CHECK(ctx.state == LAUNCH_CTRL_STATE_OFF, "part pedal for > 0.5 s -> disarmed");
    CHECK(out == 700, "part pedal -> plain driver torque");

    launch_ctrl_init(&ctx, c);
    arm(&ctx);
    in = inputs(2300, 0.0f, 0.0f, 0);
    in.brake_pressed = true;
    launch_ctrl_apply(&ctx, &in);
    CHECK(ctx.state == LAUNCH_CTRL_STATE_ARMED, "full pedal with the brake still on -> stays ARMED");
    for (int i = 0; i < 20; i++) // pedal ramps 0 -> 100% over 0.2 s
    {
        in = inputs((uint16_t)(2300 * (i + 1) / 20), 0.0f, 0.0f, i);
        launch_ctrl_apply(&ctx, &in);
    }
    CHECK(ctx.state == LAUNCH_CTRL_STATE_ACTIVE, "brake off, pedal floored within 0.5 s -> ACTIVE");

    launch_ctrl_init(&ctx, c);
    arm(&ctx);
    in = inputs(2300, 0.0f, 0.0f, 0);
    in.pedal = (uint16_t)(c->trigger_pedal - 10); // floored, but below 90%
    launch_ctrl_apply(&ctx, &in);
    CHECK(ctx.state == LAUNCH_CTRL_STATE_ARMED, "pedal below 90% does not start a launch");
    in.pedal = c->trigger_pedal;
    launch_ctrl_apply(&ctx, &in);
    CHECK(ctx.state == LAUNCH_CTRL_STATE_ACTIVE, "pedal at 90% starts the launch");

    launch_ctrl_init(&ctx, c);
    run_to_active(&ctx);
    in = inputs(2300, 2.0f, 0.0f, 0);
    in.motor_rpm = rpm_for(-1.2f); // rolling backwards -> FAULT
    launch_ctrl_apply(&ctx, &in);
    CHECK(ctx.state == LAUNCH_CTRL_STATE_FAULT, "FAULT");
    arm(&ctx);
    CHECK(ctx.state == LAUNCH_CTRL_STATE_ARMED && ctx.fault == LAUNCH_CTRL_FAULT_NONE,
          "stopped with good inputs after a fault -> re-arms and clears the fault");

    // a fault that persists (no new front samples) must not flip FAULT <-> ARMED
    launch_ctrl_init(&ctx, c);
    arm(&ctx);
    for (int i = 0; i < 20; i++) // becomes stale after front_stale_s
    {
        in = inputs(0, 0.0f, 0.0f, i);
        in.front_new_sample = false; // wheelspeed thread stopped
        launch_ctrl_apply(&ctx, &in);
    }
    CHECK(ctx.state == LAUNCH_CTRL_STATE_FAULT, "no new front samples while armed -> FAULT");
    int stayed = 1;
    for (int i = 0; i < 100; i++)
    {
        in = inputs(0, 0.0f, 0.0f, i);
        in.front_new_sample = false;
        launch_ctrl_apply(&ctx, &in);
        stayed &= (ctx.state == LAUNCH_CTRL_STATE_FAULT);
    }
    CHECK(stayed, "persistent fault while stopped -> stays FAULT, no re-arm flapping");
    arm(&ctx); // samples arriving again
    CHECK(ctx.state == LAUNCH_CTRL_STATE_ARMED, "fault gone -> re-arms");
}

static void run_to_active(launch_ctrl_context_t *ctx)
{
    arm(ctx);
    for (int i = 0; i < 5; i++)
    {
        launch_ctrl_inputs_t in = inputs(2300, 2.0f, 0.0f, i);
        launch_ctrl_apply(ctx, &in);
    }
}

static void test_faults_fall_back_to_driver(void)
{
    printf("bad sensor data -> FAULT -> crawl torque (fault_torque)\n");
    launch_ctrl_context_t ctx;

    launch_ctrl_init(&ctx, &launch_ctrl_config_default);
    run_to_active(&ctx);
    uint16_t out = 0;
    for (int i = 0; i < 20; i++)
    {
        launch_ctrl_inputs_t in = inputs(2300, 4.0f, 0.0f, i);
        in.front_new_sample = false; // wheelspeed thread stopped
        out = launch_ctrl_apply(&ctx, &in);
    }
    CHECK(ctx.fault == LAUNCH_CTRL_FAULT_FRONT_STALE, "stale front -> FRONT_STALE");
    CHECK(out == launch_ctrl_config_default.fault_torque, "fault -> capped at fault_torque (crawl), not the full request");
    for (int i = 0; i < 100; i++) // keeps driving on with bad data: stays capped
    {
        launch_ctrl_inputs_t in = inputs(2300, 6.0f, 0.0f, i);
        in.front_new_sample = false;
        out = launch_ctrl_apply(&ctx, &in);
    }
    CHECK(ctx.state == LAUNCH_CTRL_STATE_FAULT && out == launch_ctrl_config_default.fault_torque,
          "still moving -> stays FAULT at crawl torque");
    {
        launch_ctrl_inputs_t in = inputs(100, 6.0f, 0.0f, 0); // 10 Nm request, below the cap
        in.front_new_sample = false;
        CHECK(launch_ctrl_apply(&ctx, &in) == 100, "fault cap never raises a smaller request");
    }
    stand_still(&ctx, 0.2f);
    CHECK(ctx.state == LAUNCH_CTRL_STATE_ARMED, "stop + pedal released -> fault clears, re-armed");

    launch_ctrl_init(&ctx, &launch_ctrl_config_default);
    run_to_active(&ctx);
    launch_ctrl_inputs_t in = inputs(2300, 2.0f, 0.0f, 0);
    in.motor_rpm = rpm_for(-1.2f); // wrong sign convention (or rolling backwards)
    launch_ctrl_apply(&ctx, &in);
    CHECK(ctx.fault == LAUNCH_CTRL_FAULT_MOTOR_REVERSE, "negative motor rpm -> MOTOR_REVERSE");

    // logged on the car: FL read 6.8 m/s for 0.1 s with the motor still
    // stopped, as the inverter switched on at launch
    launch_ctrl_init(&ctx, &launch_ctrl_config_default);
    arm(&ctx);
    for (int i = 0; i < 10; i++)
    {
        in = inputs(2300, 0.0f, 6.8f, i);
        out = launch_ctrl_apply(&ctx, &in);
    }
    CHECK(ctx.state == LAUNCH_CTRL_STATE_ACTIVE && ctx.fault == LAUNCH_CTRL_FAULT_NONE,
          "FL spike with the motor stopped -> ignored, launch stays ACTIVE");
    CHECK(ctx.v_front == 0.0f, "motor stopped -> front speed taken as 0");
    CHECK(out < 2300, "FL spike with the motor stopped -> torque still limited");

    launch_ctrl_init(&ctx, &launch_ctrl_config_default);
    for (int i = 0; i < 510; i++)
    {
        in = inputs(0, 0.0f, (i % 7 == 0) ? 6.8f : 0.0f, i); // noisy FL while stopped
        launch_ctrl_apply(&ctx, &in);
    }
    CHECK(ctx.state == LAUNCH_CTRL_STATE_ARMED, "FL noise while stopped does not stop arming");

    // rolling at 1 m/s (motor turning) when the driver floors it, and FL
    // spikes to 6.8 m/s for 0.1 s as the inverter switches on
    launch_ctrl_init(&ctx, &launch_ctrl_config_default);
    arm(&ctx);
    for (int i = 0; i < 20; i++)
    {
        in = inputs(0, 1.0f, 0.0f, i); // brake off, rolling, pedal not pressed yet
        launch_ctrl_apply(&ctx, &in);
    }
    CHECK(ctx.state == LAUNCH_CTRL_STATE_ARMED, "rolling at 1 m/s -> still ARMED");
    float front_max = 0.0f;
    for (int i = 0; i < 10; i++)
    {
        in = inputs(2300, 1.0f, 6.8f, i);
        out = launch_ctrl_apply(&ctx, &in);
        front_max = (ctx.v_front > front_max) ? ctx.v_front : front_max;
    }
    CHECK(ctx.state == LAUNCH_CTRL_STATE_ACTIVE && ctx.fault == LAUNCH_CTRL_FAULT_NONE,
          "FL spike while rolling -> ignored, launch stays ACTIVE");
    CHECK(front_max < 0.1f, "FL spike while rolling -> front speed kept at the last good value");
    CHECK(out < 2300, "FL spike while rolling -> torque still limited");

    // FL drops to 0 for one window mid-launch, then reads normally again
    launch_ctrl_init(&ctx, &launch_ctrl_config_default);
    arm(&ctx);
    for (int i = 0; i < 100; i++)
    {
        float v = 0.5f + 0.1f * (float)i; // 0.5 -> 10.4 m/s in 1 s
        in = inputs(2300, v + 1.0f, v, i);
        if (i == 80)
            in.fl_rpm = 0.0f; // dropout
        launch_ctrl_apply(&ctx, &in);
        if (i == 80)
            CHECK(ctx.v_front > 7.0f, "FL dropout to 0 at speed -> ignored");
    }
    CHECK(ctx.v_front > 8.5f, "FL back after the dropout -> taken again");
    CHECK(ctx.fault == LAUNCH_CTRL_FAULT_NONE, "steady 10 m/s/s launch -> every FL reading taken, no fault");

    // a stuck/wrong FL that stays wrong is still caught
    launch_ctrl_init(&ctx, &launch_ctrl_config_default);
    run_to_active(&ctx);
    for (int i = 0; i < 100 && ctx.state == LAUNCH_CTRL_STATE_ACTIVE; i++)
    {
        in = inputs(2300, 2.0f, 9.0f, i); // front reads 8 m/s, rears 2 m/s, for up to 1 s
        launch_ctrl_apply(&ctx, &in);
    }
    CHECK(ctx.fault == LAUNCH_CTRL_FAULT_FRONT_AHEAD,
          "front stays much faster than rears -> FRONT_AHEAD");
}

static void test_shadow_mode(void)
{
    printf("shadow mode computes a limit but never applies it\n");
    config_launch_ctrl_t cfg = launch_ctrl_config_default;
    cfg.shadow_mode = true;
    launch_ctrl_context_t ctx;
    launch_ctrl_init(&ctx, &cfg);
    arm(&ctx);
    int ok = 1;
    for (int i = 0; i < 20; i++)
    {
        launch_ctrl_inputs_t in = inputs(2300, 0.5f * (float)i, 0.0f, i);
        ok &= (launch_ctrl_apply(&ctx, &in) == 2300);
    }
    CHECK(ok, "shadow mode returns driver torque");
    CHECK(ctx.state == LAUNCH_CTRL_STATE_ACTIVE, "shadow mode still runs the state machine");
    CHECK(ctx.limit < 2300.0f, "shadow mode still computes (and can log) a limit");
    CHECK(!launch_ctrl_is_active(&ctx), "is_active false in shadow mode, so the old limiter keeps working");
}

static void test_power_limit(void)
{
    printf("power limit: every mode, also disabled / shadow, and during a launch\n");
    config_launch_ctrl_t cfg = launch_ctrl_config_default;
    launch_ctrl_context_t ctx;

    launch_ctrl_init(&ctx, &cfg);
    launch_ctrl_inputs_t in = inputs(2000, 10.0f, 10.0f, 0);
    in.launch_mode_selected = false;
    in.electrical_power_w = 70000.0f;
    CHECK(launch_ctrl_apply(&ctx, &in) == 2000, "70 kW -> unchanged");
    in.electrical_power_w = 76000.0f;
    CHECK(launch_ctrl_apply(&ctx, &in) == 1800, "76 kW -> 200 (20 Nm) cut");
    in.electrical_power_w = 90000.0f;
    CHECK(launch_ctrl_apply(&ctx, &in) == 0, "90 kW -> 0");
    in.electrical_power_w = INFINITY;
    CHECK(launch_ctrl_apply(&ctx, &in) == 0, "power unknown -> 0");

    cfg.enable = false;
    launch_ctrl_init(&ctx, &cfg);
    in.electrical_power_w = 76000.0f;
    CHECK(launch_ctrl_apply(&ctx, &in) == 1800, "enable=false still power limits");

    cfg = launch_ctrl_config_default;
    cfg.shadow_mode = true;
    launch_ctrl_init(&ctx, &cfg);
    arm(&ctx);
    in = inputs(2300, 3.0f, 0.0f, 0);
    in.electrical_power_w = 76000.0f;
    CHECK(launch_ctrl_apply(&ctx, &in) == 2100, "shadow mode still power limits");

    launch_ctrl_init(&ctx, &launch_ctrl_config_default);
    arm(&ctx);
    for (int i = 0; i < 5; i++)
    {
        in = inputs(2300, 0.5f, 0.0f, i);
        launch_ctrl_apply(&ctx, &in);
    }
    in = inputs(2300, 0.5f, 0.0f, 5);
    in.electrical_power_w = 78000.0f; // 600 (60 Nm) cut
    uint16_t out = launch_ctrl_apply(&ctx, &in);
    float launch_out = (ctx.limit < 2300.0f) ? (float)(uint16_t)ctx.limit : 2300.0f;
    CHECK(ctx.state == LAUNCH_CTRL_STATE_ACTIVE && ctx.limit > 1000.0f && out == (uint16_t)(launch_out - 600.0f),
          "during a launch the power cut comes on top of the launch limit");
    CHECK(ctx.output == out, "output records the torque after the power limit");
}

static void test_first_command_and_ramp(void)
{
    printf("first command = torque_start, then at most ramp_up_max; capped while ARMED\n");
    const config_launch_ctrl_t *c = &launch_ctrl_config_default;
    launch_ctrl_context_t ctx;
    launch_ctrl_inputs_t in;

    launch_ctrl_init(&ctx, c);
    arm(&ctx);
    in = inputs(1800, 0.0f, 0.0f, 0);
    in.pedal = 800; // on its way to the floor, below trigger_pedal
    uint16_t out = launch_ctrl_apply(&ctx, &in);
    CHECK(ctx.state == LAUNCH_CTRL_STATE_ARMED, "pedal at 80% -> still ARMED");
    CHECK(out == c->torque_start, "ARMED, pedal on its way down -> capped at torque_start");

    in = inputs(2300, 0.0f, 0.0f, 1);
    out = launch_ctrl_apply(&ctx, &in);
    CHECK(ctx.state == LAUNCH_CTRL_STATE_ACTIVE, "floored -> ACTIVE");
    CHECK(out == c->torque_start, "first command of the launch is torque_start");

    // the car grips (rears roll with the fronts): the limit climbs, no faster than the ramp
    int ok = 1;
    uint16_t prev = out;
    for (int i = 0; i < 100; i++)
    {
        float v = 0.05f * (float)i;
        in = inputs(2300, v, v, i + 2);
        out = launch_ctrl_apply(&ctx, &in);
        if (ctx.ramping)
            ok &= ((float)out <= (float)prev + c->ramp_up_max * in.dt_s + 1.0f);
        prev = out;
    }
    CHECK(ok, "limit never rises faster than ramp_up_max during the initial build-up");
    CHECK(out > c->torque_start, "limit does rise when the wheels grip");

    // after a wheelspin cut the limit comes back as soon as the wheels grip,
    // not at ramp_up_max
    for (int i = 0; i < 30; i++)
    {
        in = inputs(2300, 9.0f, 5.0f, i); // rears spinning well past the target
        out = launch_ctrl_apply(&ctx, &in);
    }
    CHECK(!ctx.ramping, "a wheelspin cut ends the initial ramp");
    uint16_t cut = out;
    for (int i = 0; i < 3; i++) // gripping again (the controller's 30 ms speed average has to catch up)
    {
        in = inputs(2300, 5.0f, 5.0f, i);
        out = launch_ctrl_apply(&ctx, &in);
    }
    CHECK((float)out > (float)cut + 5.0f * c->ramp_up_max * in.dt_s,
          "after a cut the limit jumps back within 30 ms when the wheels grip (not ramp steps)");
    for (int i = 3; i < 10; i++)
    {
        in = inputs(2300, 5.0f, 5.0f, i);
        out = launch_ctrl_apply(&ctx, &in);
    }
    CHECK((float)out > (float)cut + 1.5f * c->ramp_up_max * 0.1f,
          "after a cut, 0.1 s of grip brings back more than ramp_up_max would");
}

/* the wheels spin at v + 4 m/s for n steps (a wheelspin cut), front at v */
static uint16_t spin(launch_ctrl_context_t *ctx, float v, int n, int *step)
{
    uint16_t out = 0;
    for (int i = 0; i < n; i++, (*step)++)
    {
        launch_ctrl_inputs_t in = inputs(2300, v + 4.0f, v, *step);
        out = launch_ctrl_apply(ctx, &in);
    }
    return out;
}

/* the rears roll with the fronts at v (grip) for n steps; returns the last output,
 * *max_step gets the biggest single rise seen above the knee */
static uint16_t grip(launch_ctrl_context_t *ctx, float v, int n, int *step, float knee, float *max_step)
{
    uint16_t out = ctx->output, prev = ctx->output;
    for (int i = 0; i < n; i++, (*step)++)
    {
        launch_ctrl_inputs_t in = inputs(2300, v, v, *step);
        out = launch_ctrl_apply(ctx, &in);
        if (max_step && ctx->integrator > knee + 1.0f && (float)out - (float)prev > *max_step)
            *max_step = (float)out - (float)prev;
        prev = out;
    }
    return out;
}

static void test_grip_memory_recovery(void)
{
    printf("after a cut: fast refill to recover_frac x the torque it slipped at, then creep\n");
    const config_launch_ctrl_t *c = &launch_ctrl_config_default;
    launch_ctrl_context_t ctx;
    launch_ctrl_inputs_t in;
    int step = 0;

    launch_ctrl_init(&ctx, c);
    arm(&ctx);
    in = inputs(2300, 0.0f, 0.0f, step++);
    launch_ctrl_apply(&ctx, &in);
    CHECK(ctx.state == LAUNCH_CTRL_STATE_ACTIVE, "floored -> ACTIVE");

    // grip for 0.5 s: the ramp builds the limit up
    for (int i = 0; i < 50; i++, step++)
    {
        float v = 0.06f * (float)i;
        in = inputs(2300, v, v, step);
        launch_ctrl_apply(&ctx, &in);
    }
    float before = ctx.limit;
    CHECK(before > c->torque_start + 500, "limit built up during 0.5 s of grip");

    // the wheels break loose: grip memory takes the limit it slipped at
    uint16_t cut = spin(&ctx, 3.0f, 20, &step);
    CHECK(fabsf(ctx.slip_torque - before) < 0.05f * before + 50.0f,
          "slip_torque = the limit when the wheels broke loose");
    CHECK(!ctx.ramping, "a wheelspin cut after ramp_min_s ends the initial ramp");
    CHECK((float)cut < before - 300.0f, "the spin is cut");
    CHECK(cut >= c->torque_floor, "never below torque_floor");

    // grip again: back to ~recover_frac x slip_torque fast (within 0.15 s)
    float knee = c->recover_frac * ctx.slip_torque;
    uint16_t back = grip(&ctx, 3.0f, 15, &step, 1e9f, NULL);
    CHECK(ctx.integrator >= knee - 1.0f, "integrator refilled to the knee within 0.15 s of grip");
    CHECK((float)back >= knee - 1.0f, "output back at (at least) the knee within 0.15 s");
    float t_needed = (knee - (float)c->torque_floor) / c->ramp_up_max;
    CHECK(t_needed > 0.15f, "(so the refill really is faster than ramp_up_max would allow)");

    // above the knee the integrator only creeps (ki_recover, capped at ramp_up_max)
    float i0 = ctx.integrator;
    grip(&ctx, 3.0f, 10, &step, knee, NULL);
    CHECK(ctx.integrator - i0 <= c->ramp_up_max * 0.1f + 1.0f, "above the knee: integrator rises at most ramp_up_max");

    // a second spin at a lower torque lowers the memory
    float lim2 = ctx.limit;
    spin(&ctx, 3.5f, 20, &step);
    CHECK(fabsf(ctx.slip_torque - lim2) < 0.05f * lim2 + 50.0f, "a new spin updates slip_torque");

    // safety: never above the driver request, even with grip memory high
    launch_ctrl_init(&ctx, c);
    arm(&ctx);
    in = inputs(2300, 0.0f, 0.0f, 0);
    launch_ctrl_apply(&ctx, &in);
    step = 1;
    for (int i = 0; i < 40; i++, step++)
    {
        in = inputs(2300, 0.05f * i, 0.05f * i, step);
        launch_ctrl_apply(&ctx, &in);
    }
    spin(&ctx, 2.0f, 15, &step);
    int ok = 1;
    for (int i = 0; i < 50; i++, step++)
    {
        in = inputs(600, 2.0f, 2.0f, step); // driver backs off to 60 Nm (still above abort_torque)
        uint16_t out = launch_ctrl_apply(&ctx, &in);
        ok &= (out <= 600);
    }
    CHECK(ok, "refill never goes above the driver request");
}

static void test_early_noise_keeps_ramp(void)
{
    printf("a cut in the first ramp_min_s (motor speed noise) doesn't end the gentle ramp\n");
    const config_launch_ctrl_t *c = &launch_ctrl_config_default;
    launch_ctrl_context_t ctx;
    launch_ctrl_inputs_t in;

    launch_ctrl_init(&ctx, c);
    arm(&ctx);
    in = inputs(2300, 0.0f, 0.0f, 0);
    launch_ctrl_apply(&ctx, &in);
    int step = 1;
    // 0.05 s in: one noisy motor sample says 2 m/s while the car has barely moved
    for (; step < 5; step++)
    {
        in = inputs(2300, 0.05f * step, 0.0f, step);
        launch_ctrl_apply(&ctx, &in);
    }
    in = inputs(2300, 2.5f, 0.0f, step++);
    launch_ctrl_apply(&ctx, &in);
    CHECK(ctx.ramping, "noise cut at 0.05 s keeps the initial ramp");

    // grip afterwards: the limit keeps to the ramp, no jump to the full request
    int ok = 1;
    float prev = ctx.limit;
    for (int i = 0; i < 15; i++, step++)
    {
        float v = 0.3f + 0.05f * i;
        in = inputs(2300, v, v, step);
        launch_ctrl_apply(&ctx, &in);
        ok &= (ctx.limit <= prev + c->ramp_up_max * 0.01f + 1.0f);
        prev = ctx.limit;
    }
    CHECK(ok, "limit still rises no faster than ramp_up_max after the noise");
    CHECK(ctx.limit < 2000.0f, "no snap to the full request in the first 0.2 s");
}

static void test_driveshaft_shake_ignored(void)
{
    printf("a 25 Hz +-1 m/s motor speed shake around steady slip doesn't make the limit chatter\n");
    const config_launch_ctrl_t *c = &launch_ctrl_config_default;
    launch_ctrl_context_t ctx;
    launch_ctrl_inputs_t in;

    launch_ctrl_init(&ctx, c);
    arm(&ctx);
    in = inputs(2300, 0.0f, 0.0f, 0);
    launch_ctrl_apply(&ctx, &in);
    int step = 1;
    for (int i = 0; i < 60; i++, step++) // build up with grip
    {
        float v = 0.08f * (float)i;
        in = inputs(2300, v, v, step);
        launch_ctrl_apply(&ctx, &in);
    }
    // car at 5 m/s, rears at 10% slip on average, shaking +-1 m/s at 25 Hz (40 ms period)
    const int shake[4] = {1, 1, -1, -1};
    float sum_dd = 0.0f, prev = -1.0f, prev2 = -1.0f;
    int n = 0, ok = 1;
    for (int i = 0; i < 200; i++, step++)
    {
        float vd = 5.5f + 1.0f * (float)shake[i % 4];
        in = inputs(2300, vd, 5.0f, step);
        uint16_t out = launch_ctrl_apply(&ctx, &in);
        ok &= (ctx.state == LAUNCH_CTRL_STATE_ACTIVE);
        if (i > 40 && prev2 >= 0.0f)
        {
            sum_dd += fabsf((float)out - 2.0f * prev + prev2) / 2.0f;
            n++;
        }
        prev2 = prev;
        prev = (float)out;
    }
    CHECK(ok, "stays ACTIVE through the shake (no fault)");
    printf("    zigzag %.1f Nm\n", sum_dd/(float)n/10.0f); printf("    zigzag %.1f Nm\n", sum_dd/(float)n/10.0f); printf("    zigzag %.1f Nm\n", sum_dd/(float)n/10.0f); CHECK(n > 0 && sum_dd / (float)n < 50.0f, "limit zig-zag under 5 Nm per step during the shake");
}

static void test_timeout_and_exit_speed(void)
{
    printf("hands back after exit speed or timeout\n");
    launch_ctrl_context_t ctx;
    launch_ctrl_init(&ctx, &launch_ctrl_config_default);
    arm(&ctx);
    float v = 0.0f;
    int i = 0;
    for (; i < 1000 && ctx.state != LAUNCH_CTRL_STATE_DONE; i++)
    {
        v += 10.0f * 0.01f;
        launch_ctrl_inputs_t in = inputs(2300, v * 1.05f + 0.1f, v, i);
        launch_ctrl_apply(&ctx, &in);
    }
    CHECK(ctx.state == LAUNCH_CTRL_STATE_DONE, "reaches DONE");
    CHECK(v > 19.0f && v < 23.0f, "DONE close to the 20 m/s exit speed");
}

int main(void)
{
    test_disabled_passes_through();
    test_arms_only_when_stationary();
    test_limits_wheelspin_from_rest();
    test_releases_when_gripping();
    test_never_exceeds_driver_fuzz();
    test_lift_off_and_brake_end_launch();
    test_arming_protocol();
    test_faults_fall_back_to_driver();
    test_shadow_mode();
    test_power_limit();
    test_first_command_and_ramp();
    test_timeout_and_exit_speed();
    test_grip_memory_recovery();
    test_early_noise_keeps_ramp();
    test_driveshaft_shake_ignored();
    printf("\nRESULT: %s (%d/%d checks)\n", failures ? "FAIL" : "PASS", checks - failures, checks);
    return failures ? 1 : 0;
}
