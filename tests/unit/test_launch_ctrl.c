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

/* motor rpm that makes the driven wheels go at v m/s */
static int16_t rpm_for(float v)
{
    return (int16_t)lroundf(v / 0.20f / (2.0f * 3.14159265f / 60.0f) * 3.18f);
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
    in.motor_rpm = rpm_for(v_driven);
    in.fl_rpm = front_rpm_for(v_front);
    in.fr_rpm = in.fl_rpm;
    in.front_new_sample = (step % 2) == 0; // 20 ms window, 10 ms loop
    in.launch_mode_selected = true;
    in.brake_pressed = false;
    return in;
}

/* sit still for a moment so the controller arms */
static void arm(launch_ctrl_context_t *ctx)
{
    for (int i = 0; i < 10; i++)
    {
        launch_ctrl_inputs_t in = inputs(0, 0.0f, 0.0f, i);
        launch_ctrl_apply(ctx, &in);
    }
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
    CHECK(ctx.state == LAUNCH_CTRL_STATE_ARMED, "stationary + mode selected -> ARMED");

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
        launch_ctrl_inputs_t in;
        in.dt_s = (rand() % 50 == 0) ? 0.0f : 0.01f;
        in.driver_torque = (uint16_t)(rand() % 2600);
        in.motor_rpm = (int16_t)(rand() % 8000 - 500);
        in.fl_rpm = (float)(rand() % 1500);
        in.fr_rpm = (rand() % 3) ? in.fl_rpm : (float)(rand() % 1500);
        in.front_new_sample = rand() % 2;
        in.launch_mode_selected = rand() % 20 != 0;
        in.brake_pressed = rand() % 30 == 0;
        in.electrical_power_w = (rand() % 50 == 0) ? INFINITY : (float)(rand() % 90000);
        if (rand() % 100 == 0)
        {
            in.driver_torque = 0;
            in.motor_rpm = 0;
            in.fl_rpm = 0;
            in.fr_rpm = 0;
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

    /* must come to a stop with the pedal released before it re-arms */
    in = inputs(2300, 0.0f, 0.0f, 12);
    launch_ctrl_apply(&ctx, &in);
    CHECK(ctx.state == LAUNCH_CTRL_STATE_DONE, "no re-arm with pedal pressed");
    arm(&ctx);
    arm(&ctx);
    CHECK(ctx.state == LAUNCH_CTRL_STATE_ARMED, "re-arms after stop + pedal released");
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
    printf("bad sensor data -> FAULT -> driver torque\n");
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
    CHECK(out == 2300, "fault -> driver torque");

    launch_ctrl_init(&ctx, &launch_ctrl_config_default);
    run_to_active(&ctx);
    launch_ctrl_inputs_t in = inputs(2300, 12.0f, 10.0f, 0);
    in.fr_rpm = in.fl_rpm * 0.3f; // FR sensor dropping out
    out = launch_ctrl_apply(&ctx, &in);
    CHECK(ctx.fault == LAUNCH_CTRL_FAULT_FRONT_DISAGREE, "FL/FR disagree -> FRONT_DISAGREE");
    CHECK(out == 2300, "fault -> driver torque");

    launch_ctrl_init(&ctx, &launch_ctrl_config_default);
    run_to_active(&ctx);
    in = inputs(2300, 2.0f, 0.0f, 0);
    in.motor_rpm = -500; // wrong sign convention
    launch_ctrl_apply(&ctx, &in);
    CHECK(ctx.fault == LAUNCH_CTRL_FAULT_MOTOR_REVERSE, "negative motor rpm -> MOTOR_REVERSE");

    launch_ctrl_init(&ctx, &launch_ctrl_config_default);
    run_to_active(&ctx);
    in = inputs(2300, 2.0f, 9.0f, 0); // front spike
    launch_ctrl_apply(&ctx, &in);
    CHECK(ctx.fault == LAUNCH_CTRL_FAULT_FRONT_AHEAD, "front much faster than rears -> FRONT_AHEAD");
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
    test_faults_fall_back_to_driver();
    test_shadow_mode();
    test_power_limit();
    test_timeout_and_exit_speed();
    printf("\nRESULT: %s (%d/%d checks)\n", failures ? "FAIL" : "PASS", checks - failures, checks);
    return failures ? 1 : 0;
}
