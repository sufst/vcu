/*
 * Unit tests for the VCU's pure logic, using the real calibration in
 * src/SUFST/Src/config.c:
 *   clip_to_range, mode switch decoding, torque map, compressor,
 *   torque limiters (slip + power), SCS validation/mapping, APPS, BPS.
 *
 * Built and run by tests/run_tests.sh. No RTOS is running: the few RTOS and
 * logging functions these modules call are stubbed below.
 */
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "apps.h"
#include "bps.h"
#include "clip_to_range.h"
#include "compressor.h"
#include "config.h"
#include "mode_switch_values.h"
#include "scs.h"
#include "torque_limiters.h"
#include "torque_map.h"

/* ---------------------------------------------------------------- stubs */

ADC_TypeDef sim_adc_inst1 = { 1 }, sim_adc_inst2 = { 2 };
ADC_HandleTypeDef hadc1 = { &sim_adc_inst1 }, hadc2 = { &sim_adc_inst2 };
UART_HandleTypeDef huart8;
USART_HandleTypeDef husart1;
GPIO_TypeDef sim_gpio[7];

static uint16_t adc1[5], adc2[1];
static bool adc_ready = true;

uint16_t adc_scan_get_average(const ADC_HandleTypeDef *hadc, uint8_t slot)
{
    return (hadc->Instance == ADC2) ? adc2[slot] : adc1[slot];
}

bool adc_scan_is_ready(const ADC_HandleTypeDef *hadc)
{
    (void)hadc;
    return adc_ready;
}

TickType_t xTaskGetTickCount(void)
{
    return 0;
}

status_t log_printf(const config_log_level_t level, const char *format, ...)
{
    (void)level;
    (void)format;
    return STATUS_OK;
}

/* ------------------------------------------------------------ framework */

static int checks, failures;

#define CHECK(cond, ...)                                                       \
    do                                                                         \
    {                                                                          \
        checks++;                                                              \
        if (!(cond))                                                           \
        {                                                                      \
            failures++;                                                        \
            printf("  FAIL: ");                                                \
            printf(__VA_ARGS__);                                               \
            printf("   (%s:%d)\n", __FILE__, __LINE__);                        \
        }                                                                      \
        else if (verbose)                                                      \
        {                                                                      \
            printf("  PASS: ");                                                \
            printf(__VA_ARGS__);                                               \
            printf("\n");                                                      \
        }                                                                      \
    } while (0)

static int verbose;

static void print_section(const char *name)
{
    printf("== %s\n", name);
}

/* ---------------------------------------------------------------- tests */

static void test_clip_to_range(void)
{
    print_section("clip_to_range");
    CHECK(clip_to_range(5, 10, 20) == 10, "below range -> min");
    CHECK(clip_to_range(25, 10, 20) == 20, "above range -> max");
    CHECK(clip_to_range(15, 10, 20) == 15, "in range unchanged");
    CHECK(clip_to_range(10, 10, 20) == 10 && clip_to_range(20, 10, 20) == 20, "edges unchanged");
    CHECK(clip_to_range(0, 0, 0) == 0 && clip_to_range(65535, 0, 2500) == 2500, "extremes");
}

static void test_mode_switch(void)
{
    print_section("mode switch decoding");
    /* lower edge of each position (0..330 = 0..3.30 V) */
    const uint16_t edge[] = { 0, 19, 56, 93, 130, 168, 206, 243, 281, 315 };
    for (int m = 1; m <= 10; m++)
    {
        CHECK(convert_mode_adc_to_discrete(edge[m - 1]) == (ctrl_mode_t)m,
              "reading %u -> mode %d", edge[m - 1], m);
        if (m > 1)
            CHECK(convert_mode_adc_to_discrete(edge[m - 1] - 1) == (ctrl_mode_t)(m - 1),
                  "reading %u -> mode %d", edge[m - 1] - 1, m - 1);
    }
    CHECK(convert_mode_adc_to_discrete(330) == CTRL_MODE_INVERTER_PROG, "3.30 V -> inverter prog");
    CHECK(convert_mode_adc_to_discrete(4095) == CTRL_MODE_INVERTER_PROG, "out of range high -> 10");
    for (uint16_t r = 0; r <= 330; r++)
    {
        ctrl_mode_t m = convert_mode_adc_to_discrete(r);
        if (m < CTRL_MODE_ENDURANCE || m > CTRL_MODE_INVERTER_PROG)
        {
            CHECK(0, "reading %u gives invalid mode %d", r, m);
            break;
        }
    }
    CHECK(convert_mode_adc_to_discrete(0) != CTRL_MODE_UNKNOWN, "never decodes to UNKNOWN");
}

static void test_torque_map(void)
{
    print_section("torque map (config: exponential 1.7, 5% deadzone)");
    const config_t *cfg = config_get();
    torque_map_t map;
    CHECK(torque_map_init(&map, &cfg->torque_map) == STATUS_OK, "init ok");

    CHECK(torque_map_apply(&map, 0) == 0, "0%% pedal -> 0");
    CHECK(torque_map_apply(&map, 50) == 0, "5%% pedal (deadzone end) -> 0");
    CHECK(torque_map_apply(&map, 1000) == cfg->torque_map.output_max,
          "100%% pedal -> output_max (%u)", torque_map_apply(&map, 1000));

    uint16_t prev = 0;
    int monotonic = 1, bounded = 1;
    for (uint16_t in = 0; in <= 1000; in++)
    {
        uint16_t t = torque_map_apply(&map, in);
        if (t < prev)
            monotonic = 0;
        if (t > cfg->torque_map.output_max)
            bounded = 0;
        prev = t;
    }
    CHECK(monotonic, "torque never decreases as the pedal goes down");
    CHECK(bounded, "torque never exceeds output_max over 0..100%%");

    uint16_t half = torque_map_apply(&map, 500);
    CHECK(half > 0 && half < 500, "50%% pedal is below linear (exponential shape): %u", half);

    torque_map_set_output_max(&map, cfg->ctrl.endurance_max_torque);
    CHECK(torque_map_apply(&map, 1000) == cfg->ctrl.endurance_max_torque,
          "endurance cap applied at full pedal (%u)", torque_map_apply(&map, 1000));
    torque_map_set_output_max(&map, cfg->ctrl.crawl_max_torque);
    CHECK(torque_map_apply(&map, 1000) == cfg->ctrl.crawl_max_torque, "crawl cap applied");
    CHECK(torque_map_apply(&map, 1500) == cfg->ctrl.crawl_max_torque,
          "input above 100%% is clamped (%u)", torque_map_apply(&map, 1500));

    config_torque_map_t lin = cfg->torque_map;
    lin.function = TORQUE_MAP_LINEAR;
    CHECK(torque_map_init(&map, &lin) == STATUS_OK, "linear map init ok");
    CHECK(torque_map_apply(&map, 50) == 0, "linear: deadzone -> 0");
    uint16_t l = torque_map_apply(&map, 525);
    CHECK(l >= 495 && l <= 505, "linear: 52.5%% pedal -> ~50%% torque (%u)", l);

    config_torque_map_t bad = cfg->torque_map;
    bad.function = (torque_map_func_e)99;
    CHECK(torque_map_init(&map, &bad) == STATUS_ERROR, "invalid map function rejected");
    CHECK(torque_map_apply(&map, 1000) == 0, "invalid map function always gives 0 torque");
}

static void test_compressor(void)
{
    print_section("compressor");
    config_compressor_t c = { .threshold = 100, .p_gain = 2, .i_gain = 1, .integral_decay = 0.5f,
                              .integral_max = 50, .integral_enabled = false };
    compressor_context_t ctx;
    compressor_init(&ctx, &c);
    CHECK(compressor_apply(&ctx, 1000, 500, false) == 1000, "gate inactive -> unchanged");
    CHECK(compressor_apply(&ctx, 1000, 90, true) == 1000, "below threshold -> unchanged");
    CHECK(compressor_apply(&ctx, 1000, 100, true) == 1000, "at threshold -> unchanged");
    CHECK(compressor_apply(&ctx, 1000, 150, true) == 900, "50 over threshold x p_gain 2 -> -100");
    CHECK(compressor_apply(&ctx, 50, 150, true) == 0, "never goes below zero");
    CHECK(compressor_apply(&ctx, 1000, INFINITY, true) == 0, "infinite measurement -> 0 (safe)");

    c.integral_enabled = true;
    c.p_gain = 0;
    compressor_init(&ctx, &c);
    uint16_t a = compressor_apply(&ctx, 1000, 110, true);
    uint16_t b = compressor_apply(&ctx, 1000, 110, true);
    CHECK(a == 990 && b < a, "integral builds up (%u then %u)", a, b);
    for (int i = 0; i < 100; i++)
        b = compressor_apply(&ctx, 1000, 1000, true);
    CHECK(b == 950, "integral clamped to integral_max (%u)", b);
    for (int i = 0; i < 60; i++)
        b = compressor_apply(&ctx, 1000, 0, true);
    CHECK(b >= 999, "integral decays when below threshold (%u)", b);
    compressor_apply(&ctx, 1000, 1000, true);
    compressor_reset(&ctx);
    CHECK(ctx.integral == 0.0f, "reset clears the integral");
}

static void test_torque_limiters(void)
{
    print_section("torque limiters (config: slip P-only 10%, power 75 kW)");
    const config_t *cfg = config_get();
    torque_limiters_context_t tl;
    torque_limiters_init(&tl, &cfg->torque_limiters);
    const float gear = cfg->torque_limiters.gear_ratio;

    CHECK(tl.min_denominator_rpm >= cfg->torque_limiters.min_motor_speed_rpm / gear - 0.01f,
          "min denominator raised to min motor speed / gear (%.1f)", tl.min_denominator_rpm);

    CHECK(torque_limiters_apply(&tl, 1000, false, (int16_t)(1000 * gear), 500, 0) == 1000,
          "slip control disabled -> no slip derate");
    CHECK(torque_limiters_apply(&tl, 1000, true, (int16_t)(1000 * gear), 1000, 0) == 1000,
          "no slip (rear == front) -> unchanged");
    CHECK(torque_limiters_apply(&tl, 1000, true, (int16_t)(1000 * gear), 500, 0) == 0,
          "100%% slip -> torque cut to 0");
    uint16_t t = torque_limiters_apply(&tl, 1000, true, (int16_t)(1000 * gear), 900, 0);
    CHECK(t < 1000 && t > 900, "11%% slip -> small derate (%u)", t);
    CHECK(torque_limiters_apply(&tl, 1000, true, 100, 10, 0) == 1000,
          "motor below min speed -> slip control inactive");
    CHECK(torque_limiters_apply(&tl, 1000, true, 3000, 20, 0) == 1000,
          "front wheels too slow to trust -> slip control inactive");
    CHECK(torque_limiters_apply(&tl, 1000, true, -3000, 500, 0) == 1000,
          "negative motor speed -> slip control inactive");

    CHECK(torque_limiters_apply(&tl, 1000, false, 0, 0, 70000) == 1000, "70 kW -> unchanged");
    CHECK(torque_limiters_apply(&tl, 1000, false, 0, 0, 76000) == 800, "76 kW -> -100 Nm*10 x2");
    CHECK(torque_limiters_apply(&tl, 1000, false, 0, 0, 80000) == 0, "80 kW -> 0");
    CHECK(torque_limiters_apply(&tl, 1000, false, 0, 0, INFINITY) == 0,
          "power unknown (inverter data missing) -> 0");
    CHECK(torque_limiters_apply(&tl, 0, false, 0, 0, 0) == 0, "0 request stays 0");
}

static void set_apps(uint16_t a1, uint16_t a2)
{
    adc1[0] = a1;
    adc2[0] = a2;
}

static void test_scs(void)
{
    print_section("SCS (BPS calibration 380..800 -> 0..1000, 50% margin)");
    const config_t *cfg = config_get();
    scs_t s;
    uint16_t r = 1234;
    CHECK(scs_create(&s, &cfg->bps.scs) == STATUS_OK, "create ok");

    adc1[1] = 380;
    CHECK(scs_read(&s, &r) == STATUS_OK && r == 0, "min ADC -> 0 (%u)", r);
    adc1[1] = 800;
    CHECK(scs_read(&s, &r) == STATUS_OK && r == 1000, "max ADC -> 1000 (%u)", r);
    adc1[1] = 590;
    CHECK(scs_read(&s, &r) == STATUS_OK && r >= 495 && r <= 505, "mid ADC -> ~500 (%u)", r);
    adc1[1] = 900;
    CHECK(scs_read(&s, &r) == STATUS_OK && r == 1000 &&
              s.status_verbose == STATUS_THRESHOLD_WARNING,
          "slightly above max -> warning, clipped to 1000");
    adc1[1] = 4000;
    r = 1234;
    CHECK(scs_read(&s, &r) == STATUS_ERROR && s.status_verbose == STATUS_THRESHOLD_ERROR,
          "far above max -> error");
    CHECK(s.mapped_reading == 0 && !s.is_valid, "error -> mapped reading set to min, invalid");
    adc1[1] = 0;
    CHECK(scs_read(&s, &r) == STATUS_ERROR, "shorted to ground (0) -> error");
    adc1[1] = 590;
    CHECK(scs_read(&s, &r) == STATUS_OK, "recovers when back in range");
    adc_ready = false;
    CHECK(scs_read(&s, &r) == STATUS_ERROR, "ADC scan not ready -> error");
    adc_ready = true;
}

static void test_apps(void)
{
    print_section("APPS (inverted, two sensors, max discrepancy 120)");
    const config_t *cfg = config_get();
    apps_context_t apps;
    uint16_t r = 0;
    CHECK(apps_init(&apps, &cfg->apps) == STATUS_OK, "init ok");

    set_apps(1635, 2416);
    CHECK(apps_read(&apps, &r) == STATUS_OK && r == 0, "released -> 0 (%u)", r);
    set_apps(98, 792);
    CHECK(apps_read(&apps, &r) == STATUS_OK && r == 1000, "fully pressed -> 1000 (%u)", r);
    set_apps(1635 - (1635 - 98) / 2, 2416 - (2416 - 792) / 2);
    CHECK(apps_read(&apps, &r) == STATUS_OK && r >= 490 && r <= 510, "half -> ~500 (%u)", r);
    CHECK(apps.scs_error == SCS_ERROR_NONE, "no error flags when healthy");

    set_apps(1635, 792); /* sensor 1 released, sensor 2 fully pressed */
    r = 77;
    CHECK(apps_read(&apps, &r) == STATUS_ERROR && r == 0, "sensors disagree -> error, 0 torque");
    CHECK(apps.scs_error & SCS_ERROR_APPS_DISCREPANCY, "discrepancy flag set");

    set_apps(1635 - 200, 2416); /* ~13% apart */
    CHECK(apps_read(&apps, &r) == STATUS_ERROR, "13%% discrepancy -> error");
    set_apps(1635 - 100, 2416 - 50); /* ~3.4% apart */
    CHECK(apps_read(&apps, &r) == STATUS_OK, "small discrepancy (%u) tolerated", r);

    set_apps(4000, 2416);
    CHECK(apps_read(&apps, &r) == STATUS_ERROR && (apps.scs_error & SCS_ERROR_APPS1) && r == 0,
          "APPS1 out of range -> APPS1 flag, 0");
    set_apps(1635, 0);
    CHECK(apps_read(&apps, &r) == STATUS_ERROR && (apps.scs_error & SCS_ERROR_APPS2) && r == 0,
          "APPS2 shorted to ground -> APPS2 flag, 0");
    set_apps(4095, 4095);
    CHECK(apps_read(&apps, &r) == STATUS_ERROR && r == 0, "both unplugged (pulled high) -> 0");
    set_apps(1635, 2416);
    CHECK(apps_read(&apps, &r) == STATUS_OK && apps.scs_error == SCS_ERROR_NONE,
          "recovers and clears flags");
}

static void test_bps(void)
{
    print_section("BPS");
    const config_t *cfg = config_get();
    bps_context_t bps;
    uint16_t r = 0;
    CHECK(bps_init(&bps, &cfg->bps) == STATUS_OK, "init ok");
    adc1[1] = 380;
    CHECK(bps_read(&bps, &r) == STATUS_OK && r == 0, "no pressure -> 0");
    adc1[1] = 700;
    CHECK(bps_read(&bps, &r) == STATUS_OK && r > cfg->ctrl.bps_on_threshold,
          "pressed -> above R2D brake threshold (%u)", r);
    adc1[1] = 4095;
    CHECK(bps_read(&bps, &r) == STATUS_ERROR, "sensor fault -> error");
}

int main(int argc, char **argv)
{
    verbose = (argc > 1);
    test_clip_to_range();
    test_mode_switch();
    test_torque_map();
    test_compressor();
    test_torque_limiters();
    test_scs();
    test_apps();
    test_bps();
    printf("RESULT: %d/%d unit checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
