/*
 * VCU host simulation: runs the real VCU application (src/SUFST, RTCAN,
 * FatFs, FreeRTOS kernel) on a PC using the FreeRTOS POSIX port and a fake
 * HAL. Each scenario plays the driver, the dash, the shutdown circuit, the
 * PM100 inverter, the remote control and the CAN buses, and checks how the
 * VCU reacts.
 *
 * Usage:  vcu_sim <scenario>     run one scenario (exit code 0 = all passed)
 *         vcu_sim --list         list the scenarios
 *
 * Each scenario runs in a fresh process (tests/run_tests.sh runs them all).
 * Time is real time: 1 FreeRTOS tick = 1 ms.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>

#include <can_s.h>
#include <can_t.h>

#include "sim.h"
#include "vcu.h"

vcu_context_t vcu;

/* ================================================================ checks */

static int failures;
static int checks;

#define CHECK(cond, ...)                                                       \
    do                                                                         \
    {                                                                          \
        checks++;                                                              \
        if (cond)                                                              \
        {                                                                      \
            printf("  PASS: ");                                                \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            printf("  FAIL: ");                                                \
            failures++;                                                        \
        }                                                                      \
        printf(__VA_ARGS__);                                                   \
        printf("\n");                                                          \
        fflush(stdout);                                                        \
    } while (0)

#define NOTE(...)                                                              \
    do                                                                         \
    {                                                                          \
        printf("  NOTE: ");                                                    \
        printf(__VA_ARGS__);                                                   \
        printf("\n");                                                          \
        fflush(stdout);                                                        \
    } while (0)

static const char *state_names[] = {
    "TS_BUTTON_WAIT", "WAIT_NEG_AIR", "PRECHARGE_WAIT", "R2D_WAIT",
    "TS_ON", "R2D_OFF", "R2D_OFF_WAIT", "TS_ACTIVATION_FAILURE",
    "TS_RUN_FAULT", "SPIN", "APPS_SCS_FAULT", "APPS_BPS_FAULT",
};

static const char *sname(ctrl_state_t s)
{
    return (s < sizeof(state_names) / sizeof(state_names[0])) ? state_names[s] : "?";
}

/* ========================================================== inverter model */

typedef struct
{
    volatile int broadcasting;   // PM100 powered and sending its broadcasts
    volatile uint8_t vsm;        // VSM state: 0 start, 3 precharge complete, 6 running
    volatile uint8_t lockout;    // inverter enable lockout
    volatile uint16_t run_lo, run_hi, post_lo, post_hi; // fault words
    volatile int16_t motor_rpm;
    volatile double dc_voltage, dc_current;
    volatile double module_temp, motor_temp;
} pm100_model_t;

static pm100_model_t inv = { .dc_voltage = 0.0, .module_temp = 25.0, .motor_temp = 25.0 };

/* ============================================================ bus monitor */

static volatile unsigned torque_cmds, disable_cmds, wheel_rpm_frames, vcu_state_frames, pdm_frames;
static volatile int last_torque = -1, last_enable = -1, last_direction = -1;
static volatile int last_bc_state = -1, last_bc_cur_mode = -1;
static volatile int pdm_inverter = -1, pdm_brakelight = -1, pdm_pump = -1, pdm_fan = -1;
static volatile int bc_ctrl_error = -1, bc_pm100_error = -1;
static volatile TickType_t last_torque_cmd_tick;
static volatile TickType_t max_torque_cmd_gap;
static volatile int track_gaps;

/* HAL callbacks normally routed in freertos.c */
void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *h)
{
    vcu_handle_can_tx_mailbox_callback(&vcu, h);
}

void sim_on_tx(int bus, uint32_t id, const uint8_t *data, uint8_t len)
{
    (void)bus;
    if (id == CAN_T_PM100_COMMAND_MESSAGE_FRAME_ID)
    {
        static const uint8_t zeros[8] = { 0 };
        if (memcmp(data, zeros, len) == 0)
        {
            disable_cmds++;
        }
        struct can_t_pm100_command_message_t cmd;
        can_t_pm100_command_message_unpack(&cmd, data, len);
        last_torque = cmd.pm100_torque_command;
        last_enable = cmd.pm100_inverter_enable;
        last_direction = cmd.pm100_direction_command;
        torque_cmds++;
        TickType_t now = xTaskGetTickCount();
        if (track_gaps && last_torque_cmd_tick && now - last_torque_cmd_tick > max_torque_cmd_gap)
            max_torque_cmd_gap = now - last_torque_cmd_tick;
        last_torque_cmd_tick = now;
    }
    else if (id == CAN_S_WHEEL_RPM_FRAME_ID)
    {
        wheel_rpm_frames++;
    }
    else if (id == CAN_T_VCU_STATE_FRAME_ID)
    {
        struct can_t_vcu_state_t st;
        can_t_vcu_state_unpack(&st, data, len);
        last_bc_state = st.vcu_ctrl_state;
        last_bc_cur_mode = st.vcu_current_mode;
        vcu_state_frames++;
    }
    else if (id == CAN_T_VCU_PDM_FRAME_ID)
    {
        struct can_t_vcu_pdm_t p;
        can_t_vcu_pdm_unpack(&p, data, len);
        pdm_inverter = p.inverter;
        pdm_brakelight = p.brakelight;
        pdm_pump = p.pump;
        pdm_fan = p.fan;
        pdm_frames++;
    }
    else if (id == CAN_T_VCU_ERROR_FRAME_ID)
    {
        struct can_t_vcu_error_t e;
        can_t_vcu_error_unpack(&e, data, len);
        bc_ctrl_error = e.vcu_ctrl_error;
        bc_pm100_error = e.vcu_pm100_error;
    }
}

/* bus 2 = CAN-T (tractive / critical), bus 1 = CAN-S (sensors) */
static void inject(int bus, uint32_t id, const uint8_t *data, uint8_t len)
{
    // in the real system this runs in the CAN RX interrupt
    sim_push_rx(bus, id, data, len);
    vcu_handle_can_rx_it(&vcu, bus == 2 ? &hcan2 : &hcan1, 0);
}

/* ================================================================= models */

/* PM100 broadcasts at 100 Hz once powered */
static void pm100_task(void *arg)
{
    (void)arg;
    TickType_t last = xTaskGetTickCount();
    for (;;)
    {
        vTaskDelayUntil(&last, 10);
        if (!inv.broadcasting)
            continue;

        uint8_t buf[8];
        struct can_t_pm100_internal_states_t st = { 0 };
        st.pm100_vsm_state = inv.vsm;
        st.pm100_inverter_enable_lockout = inv.lockout;
        can_t_pm100_internal_states_pack(buf, &st, sizeof(buf));
        inject(2, CAN_T_PM100_INTERNAL_STATES_FRAME_ID, buf, 8);

        struct can_t_pm100_fault_codes_t fc = { 0 };
        fc.pm100_run_fault_lo = inv.run_lo;
        fc.pm100_run_fault_hi = inv.run_hi;
        fc.pm100_post_fault_lo = inv.post_lo;
        fc.pm100_post_fault_hi = inv.post_hi;
        can_t_pm100_fault_codes_pack(buf, &fc, sizeof(buf));
        inject(2, CAN_T_PM100_FAULT_CODES_FRAME_ID, buf, 8);

        struct can_t_pm100_motor_position_info_t mp = { 0 };
        mp.pm100_motor_speed = inv.motor_rpm;
        can_t_pm100_motor_position_info_pack(buf, &mp, sizeof(buf));
        inject(2, CAN_T_PM100_MOTOR_POSITION_INFO_FRAME_ID, buf, 8);

        struct can_t_pm100_voltage_info_t vi = { 0 };
        vi.pm100_dc_bus_voltage = can_t_pm100_voltage_info_pm100_dc_bus_voltage_encode(inv.dc_voltage);
        can_t_pm100_voltage_info_pack(buf, &vi, sizeof(buf));
        inject(2, CAN_T_PM100_VOLTAGE_INFO_FRAME_ID, buf, 8);

        struct can_t_pm100_current_info_t ci = { 0 };
        ci.pm100_dc_bus_current = can_t_pm100_current_info_pm100_dc_bus_current_encode(inv.dc_current);
        can_t_pm100_current_info_pack(buf, &ci, sizeof(buf));
        inject(2, CAN_T_PM100_CURRENT_INFO_FRAME_ID, buf, 8);

        struct can_t_pm100_temperature_set_1_t t1 = { 0 };
        t1.pm100_module_a = can_t_pm100_temperature_set_1_pm100_module_a_encode(inv.module_temp);
        can_t_pm100_temperature_set_1_pack(buf, &t1, sizeof(buf));
        inject(2, CAN_T_PM100_TEMPERATURE_SET_1_FRAME_ID, buf, 8);

        struct can_t_pm100_temperature_set_3_t t3 = { 0 };
        t3.pm100_motor_temperature = can_t_pm100_temperature_set_3_pm100_motor_temperature_encode(inv.motor_temp);
        can_t_pm100_temperature_set_3_pack(buf, &t3, sizeof(buf));
        inject(2, CAN_T_PM100_TEMPERATURE_SET_3_FRAME_ID, buf, 8);
    }
}

/* wheel speed sensor pulses: wheel_period_ms between pulses on each front wheel */
static volatile int wheel_period_ms = 5;
static void wheel_task(void *arg)
{
    (void)arg;
    for (;;)
    {
        vTaskDelay(wheel_period_ms > 0 ? wheel_period_ms : 50);
        if (wheel_period_ms <= 0)
            continue;
        vcu_handle_gpio_exti(&vcu, WHEELSPEED_FL_Pin);
        vcu_handle_gpio_exti(&vcu, WHEELSPEED_FR_Pin);
        vcu_handle_gpio_exti(&vcu, WHEELSPEED_RL_Pin);
        vcu_handle_gpio_exti(&vcu, WHEELSPEED_RR_Pin);
    }
}

/* remote control unit (CAN-S, 100 Hz while active) */
static volatile int remote_active;
static volatile uint16_t remote_torque;
static volatile uint8_t remote_r2d, remote_ts_on;
static void remote_task(void *arg)
{
    (void)arg;
    for (;;)
    {
        vTaskDelay(10);
        if (!remote_active)
            continue;
        struct can_s_vcu_simulation_t m = { .sim_torque_request = remote_torque,
                                            .sim_r2_d = remote_r2d,
                                            .sim_ts_on = remote_ts_on };
        uint8_t buf[8] = { 0 };
        can_s_vcu_simulation_pack(buf, &m, sizeof(buf));
        inject(1, CAN_S_VCU_SIMULATION_FRAME_ID, buf, CAN_S_VCU_SIMULATION_LENGTH);
    }
}

/* dash fan switch broadcast (CAN-S, 10 Hz while active) */
static volatile int dash_fan_active, dash_fan_switch;
static void dash_fan_task(void *arg)
{
    (void)arg;
    for (;;)
    {
        vTaskDelay(100);
        if (!dash_fan_active)
            continue;
        struct can_s_dash_sensors_digital_t d = { 0 };
        d.fan_switch = (uint8_t)dash_fan_switch;
        uint8_t buf[8] = { 0 };
        can_s_dash_sensors_digital_pack(buf, &d, sizeof(buf));
        inject(1, CAN_S_DASH_SENSORS_DIGITAL_FRAME_ID, buf, CAN_S_DASH_SENSORS_DIGITAL_LENGTH);
    }
}

/* prints every control state change */
static void monitor_task(void *arg)
{
    (void)arg;
    ctrl_state_t prev = vcu.ctrl.state;
    for (;;)
    {
        vTaskDelay(1);
        ctrl_state_t now = vcu.ctrl.state;
        if (now != prev)
        {
            printf("  [%6lu ms] state %s -> %s\n", (unsigned long)xTaskGetTickCount(),
                   sname(prev), sname(now));
            fflush(stdout);
            prev = now;
        }
    }
}

/* =============================================================== helpers */

/* digital inputs on the VCU are inverted: active = pin LOW */
static void set_active(GPIO_TypeDef *port, uint16_t pin, int active)
{
    sim_set_input(port, pin, active ? 0 : 1);
}

/* outputs are inverted too: 'on' = pin LOW (except the RTDS) */
static int output_on(GPIO_TypeDef *port, uint16_t pin)
{
    return sim_get_output(port, pin) == 0;
}

/* dash LEDs are active-low, so the LED is lit when the output pin is SET */
static int led_lit(GPIO_TypeDef *port, uint16_t pin)
{
    return sim_get_output(port, pin) == 1;
}

static void press(GPIO_TypeDef *port, uint16_t pin, uint32_t ms)
{
    set_active(port, pin, 1);
    vTaskDelay(ms);
    set_active(port, pin, 0);
}

static void press_ts(void)
{
    press(TS_ON_BTN_GPIO_Port, TS_ON_BTN_Pin, 700);
}

static void press_r2d(void)
{
    press(R2D_BTN_GPIO_Port, R2D_BTN_Pin, 700);
}

static void set_shutdown_closed(int closed)
{
    set_active(TS_READY_GPIO_Port, TS_READY_Pin, closed);
}

/* pedal position 0..1000 per mille -> raw ADC (config: inverted APPS) */
static void set_pedal(int permille)
{
    sim_adc1[0] = (uint16_t)(1635 - (permille * (1635 - 98)) / 1000);
    sim_adc2[0] = (uint16_t)(2416 - (permille * (2416 - 792)) / 1000);
}

/* raw brake pressure ADC: 380 = none, 800 = full */
static void set_brake(int raw)
{
    sim_adc1[1] = (uint16_t)raw;
}

#define BRAKE_OFF 380
#define BRAKE_ON 700

/* mode switch: middle of each of the 10 positions (0..3300 raw) */
static void set_mode(ctrl_mode_t m)
{
    static const uint16_t mid[] = { 0, 100, 370, 740, 1110, 1490, 1870, 2240, 2620, 2980, 3220 };
    sim_adc1[4] = mid[m];
}

static int wait_state(ctrl_state_t s, uint32_t timeout_ms)
{
    for (uint32_t t = 0; t < timeout_ms; t++)
    {
        if (vcu.ctrl.state == s)
            return 1;
        vTaskDelay(1);
    }
    return vcu.ctrl.state == s;
}

/* returns the number of ms it took to reach state s, or -1 */
static int time_to_state(ctrl_state_t s, uint32_t timeout_ms)
{
    for (uint32_t t = 0; t <= timeout_ms; t++)
    {
        if (vcu.ctrl.state == s)
            return (int)t;
        vTaskDelay(1);
    }
    return -1;
}

static int stays_in_state(ctrl_state_t s, uint32_t ms)
{
    for (uint32_t t = 0; t < ms; t++)
    {
        if (vcu.ctrl.state != s)
            return 0;
        vTaskDelay(1);
    }
    return 1;
}

static int count_toggles(GPIO_TypeDef *port, uint16_t pin, uint32_t ms)
{
    int toggles = 0;
    int prev = sim_get_output(port, pin);
    for (uint32_t i = 0; i < ms; i++)
    {
        vTaskDelay(1);
        int now = sim_get_output(port, pin);
        if (now != prev)
            toggles++;
        prev = now;
    }
    return toggles;
}

static int is_ts_fault(void)
{
    return vcu.ctrl.state == CTRL_STATE_TS_RUN_FAULT ||
        vcu.ctrl.state == CTRL_STATE_TS_ACTIVATION_FAILURE;
}

static int wait_ts_fault(uint32_t timeout_ms)
{
    for (uint32_t t = 0; t < timeout_ms; t++)
    {
        if (is_ts_fault())
            return 1;
        vTaskDelay(1);
    }
    return is_ts_fault();
}

/* inverter powered and broadcasting, HV not yet precharged */
static void inverter_power_up(void)
{
    inv.vsm = 0;
    inv.broadcasting = 1;
    inv.dc_voltage = 0;
}

/* TS button -> AIRs -> precharge -> R2D_WAIT (shutdown closed, inverter healthy) */
static int activate_ts(void)
{
    set_shutdown_closed(1);
    inverter_power_up();
    press_ts();
    if (!wait_state(CTRL_STATE_PRECHARGE_WAIT, 500))
        return 0;
    vTaskDelay(100);
    inv.vsm = 3; // precharge complete
    inv.dc_voltage = 400;
    return wait_state(CTRL_STATE_R2D_WAIT, 300);
}

/* brake + R2D -> RTDS -> TS_ON */
static int enter_r2d(void)
{
    set_brake(BRAKE_ON);
    vTaskDelay(50);
    press_r2d();
    int ok = wait_state(CTRL_STATE_TS_ON, 3000);
    set_brake(BRAKE_OFF);
    inv.vsm = 6; // running
    vTaskDelay(50);
    return ok;
}

static int drive_ready(void)
{
    return activate_ts() && enter_r2d();
}

static int torque_after(int pedal, uint32_t ms)
{
    set_pedal(pedal);
    vTaskDelay(ms);
    return last_torque;
}

static int ack_fault(void)
{
    press_ts();
    return wait_state(CTRL_STATE_TS_BUTTON_WAIT, 200);
}

/* ============================================================= scenarios */

static void sc_boot(void)
{
    vTaskDelay(500);
    CHECK(led_lit(VC_LEDS_GPIO_Port, VC_LEDS_Pin), "visual check: dash LEDs on at boot");
    vTaskDelay(500);
    unsigned st0 = vcu_state_frames, pdm0 = pdm_frames, wh0 = wheel_rpm_frames;
    int hb = count_toggles(ROB_LED_GPIO_Port, ROB_LED_Pin, 1000);
    CHECK(vcu.ctrl.state == CTRL_STATE_TS_BUTTON_WAIT, "idle in TS_BUTTON_WAIT after boot");
    CHECK(hb >= 3 && hb <= 5, "heartbeat LED blinking at 2 Hz (%d toggles in 1 s)", hb);
    CHECK(vcu_state_frames - st0 >= 9 && vcu_state_frames - st0 <= 11,
          "VCU state broadcast at 10 Hz (%u in 1 s)", vcu_state_frames - st0);
    CHECK(pdm_frames - pdm0 >= 9, "PDM frame broadcast at 10 Hz (%u in 1 s)", pdm_frames - pdm0);
    CHECK(wheel_rpm_frames - wh0 >= 45 && wheel_rpm_frames - wh0 <= 55,
          "wheel speeds broadcast at 50 Hz (%u in 1 s)", wheel_rpm_frames - wh0);
    CHECK(last_bc_state == CTRL_STATE_TS_BUTTON_WAIT, "broadcast state is TS_BUTTON_WAIT");
    CHECK(vcu.wheelspeed.last_reading.fl_rpm > 200.0f && vcu.wheelspeed.last_reading.fl_rpm < 300.0f,
          "wheel speed computed: 200 pulses/s / 48 per rev = 250 rpm (%.0f)",
          vcu.wheelspeed.last_reading.fl_rpm);
    CHECK(sim_log_lines > 0, "log service printing over UART (%u lines)", sim_log_lines);
    CHECK(!output_on(TS_ON_GPIO_Port, TS_ON_Pin), "TS_ON (AIR) output off");
    CHECK(pdm_inverter == 0 && pdm_pump == 0, "inverter and pump power off");
    CHECK(sim_get_output(R2D_SIREN_GPIO_Port, R2D_SIREN_Pin) == 0, "RTDS silent");
    vTaskDelay(1200);
    CHECK(!led_lit(VC_LEDS_GPIO_Port, VC_LEDS_Pin), "visual check finished after 2 s");
    CHECK(vcu.ctrl.error == 0 && vcu.pm100.error == 0, "no errors");
}

static void sc_ts_activation(void)
{
    vTaskDelay(500);
    set_shutdown_closed(1);
    press_ts();
    CHECK(wait_state(CTRL_STATE_WAIT_NEG_AIR, 50), "-> WAIT_NEG_AIR on TS button release");
    CHECK(output_on(TS_ON_GPIO_Port, TS_ON_Pin), "TS_ON (AIR) output on");
    inverter_power_up();
    int t = time_to_state(CTRL_STATE_PRECHARGE_WAIT, 500);
    CHECK(t >= 200 && t <= 300, "-> PRECHARGE_WAIT after 250 ms for the negative AIR (%d ms)", t);
    CHECK(vcu.ctrl.inverter_pwr, "inverter power requested");
    vTaskDelay(200);
    CHECK(pdm_inverter == 1, "inverter power on in the PDM frame");
    CHECK(stays_in_state(CTRL_STATE_PRECHARGE_WAIT, 500), "stays in PRECHARGE_WAIT until precharged");
    inv.vsm = 3;
    CHECK(wait_state(CTRL_STATE_R2D_WAIT, 200), "-> R2D_WAIT when PM100 reports precharge complete");
    CHECK(stays_in_state(CTRL_STATE_R2D_WAIT, 1000), "stays in R2D_WAIT without driver input");
}

static void sc_ts_short_press(void)
{
    vTaskDelay(500);
    set_shutdown_closed(1);
    press(TS_ON_BTN_GPIO_Port, TS_ON_BTN_Pin, 300);
    CHECK(stays_in_state(CTRL_STATE_TS_BUTTON_WAIT, 300), "0.3 s TS press ignored (needs > 0.5 s)");
    set_active(TS_ON_BTN_GPIO_Port, TS_ON_BTN_Pin, 1);
    vTaskDelay(1500);
    CHECK(vcu.ctrl.state == CTRL_STATE_TS_BUTTON_WAIT, "nothing happens while the button is held");
    set_active(TS_ON_BTN_GPIO_Port, TS_ON_BTN_Pin, 0);
    CHECK(wait_state(CTRL_STATE_WAIT_NEG_AIR, 50), "activates on release after a long hold");
}

static void sc_ts_shutdown_open(void)
{
    vTaskDelay(500);
    set_shutdown_closed(0);
    press_ts();
    CHECK(stays_in_state(CTRL_STATE_TS_BUTTON_WAIT, 500), "TS press ignored with shutdown open");
    CHECK(!output_on(TS_ON_GPIO_Port, TS_ON_Pin), "AIRs stay open");
    set_shutdown_closed(1);
    vTaskDelay(50);
    CHECK(vcu.ctrl.state == CTRL_STATE_TS_BUTTON_WAIT, "closing shutdown alone does not activate");
    press_ts();
    CHECK(wait_state(CTRL_STATE_WAIT_NEG_AIR, 50), "activates on the next press");
}

static void sc_precharge_timeout(void)
{
    vTaskDelay(500);
    set_shutdown_closed(1);
    inverter_power_up(); // broadcasting but never finishes precharge
    press_ts();
    CHECK(wait_state(CTRL_STATE_PRECHARGE_WAIT, 500), "-> PRECHARGE_WAIT");
    int t = 0;
    while (!is_ts_fault() && t < 4000)
    {
        vTaskDelay(1);
        t++;
    }
    CHECK(is_ts_fault() && t >= 2900 && t <= 3200, "TS fault after the 3 s precharge timeout (%d ms)", t);
    CHECK(vcu.ctrl.error & CTRL_ERROR_PRECHARGE_TIMEOUT, "precharge timeout error flag set");
    vTaskDelay(200);
    CHECK(!output_on(TS_ON_GPIO_Port, TS_ON_Pin), "AIRs opened");
    CHECK(bc_ctrl_error > 0, "error broadcast on CAN (0x%x)", bc_ctrl_error);
    CHECK(ack_fault(), "acknowledged with the TS button -> TS_BUTTON_WAIT");
    CHECK(vcu.ctrl.error == 0, "error cleared");
}

static void sc_inverter_silent(void)
{
    vTaskDelay(500);
    set_shutdown_closed(1);
    inv.broadcasting = 0; // inverter dead / CAN-T disconnected
    press_ts();
    CHECK(wait_state(CTRL_STATE_PRECHARGE_WAIT, 500), "-> PRECHARGE_WAIT");
    CHECK(wait_ts_fault(3500), "TS fault (precharge timeout) when the inverter never talks");
    vTaskDelay(50); // the fault handler runs on the next control tick
    CHECK(!output_on(TS_ON_GPIO_Port, TS_ON_Pin), "AIRs opened");
}

static void sc_r2d_needs_brake(void)
{
    vTaskDelay(500);
    CHECK(activate_ts(), "TS active, in R2D_WAIT");
    int r2d_led_waiting = sim_get_output(R2D_LED_GPIO_Port, R2D_LED_Pin);
    set_brake(BRAKE_OFF);
    press_r2d();
    CHECK(stays_in_state(CTRL_STATE_R2D_WAIT, 500), "R2D without brake ignored");
    CHECK(sim_get_output(R2D_SIREN_GPIO_Port, R2D_SIREN_Pin) == 0, "RTDS silent");
    set_brake(BRAKE_ON);
    press(R2D_BTN_GPIO_Port, R2D_BTN_Pin, 300);
    CHECK(stays_in_state(CTRL_STATE_R2D_WAIT, 300), "0.3 s R2D press ignored even with brake");
    press_r2d();
    vTaskDelay(300);
    CHECK(sim_get_output(R2D_SIREN_GPIO_Port, R2D_SIREN_Pin) == 1, "RTDS sounding");
    CHECK(last_enable == 0 || torque_cmds == 0 || disable_cmds > 0,
          "inverter disabled while the RTDS sounds");
    int t = time_to_state(CTRL_STATE_TS_ON, 2500);
    CHECK(t >= 1500 && t <= 1800, "-> TS_ON when the 2 s RTDS ends (%d ms later)", t);
    CHECK(sim_get_output(R2D_SIREN_GPIO_Port, R2D_SIREN_Pin) == 0, "RTDS off");
    CHECK(sim_get_output(R2D_LED_GPIO_Port, R2D_LED_Pin) != r2d_led_waiting,
          "R2D LED changes when driving starts");
    vTaskDelay(200);
    CHECK(pdm_pump == 1, "pump on while driving");
}

static void sc_ts_off_from_r2d_wait(void)
{
    vTaskDelay(500);
    CHECK(activate_ts(), "TS active, in R2D_WAIT");
    press_ts();
    CHECK(wait_state(CTRL_STATE_TS_BUTTON_WAIT, 100), "TS button in R2D_WAIT -> TS_BUTTON_WAIT");
    CHECK(!output_on(TS_ON_GPIO_Port, TS_ON_Pin), "AIRs opened");
    CHECK(!vcu.ctrl.inverter_pwr, "inverter power off");
    CHECK(activate_ts(), "can activate again afterwards");
}

static void sc_shutdown_opens_r2d_wait(void)
{
    vTaskDelay(500);
    CHECK(activate_ts(), "TS active, in R2D_WAIT");
    set_shutdown_closed(0);
    CHECK(wait_ts_fault(60), "shutdown opening -> TS fault within 60 ms");
    vTaskDelay(100);
    CHECK(!output_on(TS_ON_GPIO_Port, TS_ON_Pin), "AIRs opened");
    int toggles = count_toggles(TS_ON_LED_GPIO_Port, TS_ON_LED_Pin, 1000);
    CHECK(toggles >= 8, "TS on LED blinking in the fault (%d toggles in 1 s)", toggles);
    CHECK(vcu.ctrl.state == CTRL_STATE_TS_RUN_FAULT, "stays in the fault while not acknowledged");
    set_shutdown_closed(1);
    CHECK(ack_fault(), "acknowledged with the TS button");
    CHECK(activate_ts(), "TS can be activated again");
}

static void sc_driving_torque(void)
{
    vTaskDelay(500);
    CHECK(drive_ready(), "driving (TS_ON, endurance mode)");
    track_gaps = 1;
    int t0 = torque_after(0, 200);
    CHECK(t0 == 0, "0%% pedal -> 0 torque (%d)", t0);
    CHECK(torque_after(40, 200) == 0, "4%% pedal (deadzone) -> 0 torque");
    int prev = 0, monotonic = 1;
    for (int p = 100; p <= 1000; p += 100)
    {
        int t = torque_after(p, 100);
        if (t < prev)
            monotonic = 0;
        prev = t;
    }
    CHECK(monotonic, "torque rises with the pedal (10%%..100%%)");
    CHECK(prev == 1100, "100%% pedal -> endurance cap 110.0 Nm (%d)", prev);
    unsigned n0 = torque_cmds;
    vTaskDelay(1000);
    CHECK(torque_cmds - n0 >= 95 && torque_cmds - n0 <= 105, "torque commands at 100 Hz (%u in 1 s)",
          torque_cmds - n0);
    CHECK(max_torque_cmd_gap <= 25, "no gap in torque commands > 25 ms (max %lu ms)",
          (unsigned long)max_torque_cmd_gap);
    CHECK(last_enable == 1, "inverter enabled");
    CHECK(last_direction == 0, "direction bit = 0 (motor wired backwards, so 'reverse' = forward)");
    set_pedal(0);
    inv.motor_rpm = 0;
    vTaskDelay(100);
    CHECK(last_torque == 0 && last_enable == 0, "pedal released at standstill -> inverter disabled");
    inv.motor_rpm = 500;
    vTaskDelay(100);
    CHECK(last_enable == 1, "pedal released while rolling -> inverter stays enabled");
    inv.motor_rpm = -5;
    vTaskDelay(100);
    CHECK(last_enable == 0, "standstill with -5 rpm sensor jitter -> inverter disabled");
    inv.motor_rpm = -500;
    vTaskDelay(100);
    CHECK(last_enable == 1, "rolling at -500 rpm -> inverter stays enabled");
}

static void sc_brake_light(void)
{
    vTaskDelay(500);
    set_brake(BRAKE_OFF);
    vTaskDelay(200);
    CHECK(pdm_brakelight == 0, "brake light off with no pressure");
    set_brake(BRAKE_ON);
    vTaskDelay(150);
    CHECK(pdm_brakelight == 0, "not on before the 0.3 s debounce");
    vTaskDelay(400);
    CHECK(pdm_brakelight == 1, "brake light on after 0.3 s of braking");
    set_brake(BRAKE_OFF);
    vTaskDelay(200);
    CHECK(pdm_brakelight == 0, "off when released");
    set_brake(BRAKE_ON);
    vTaskDelay(100);
    set_brake(BRAKE_OFF);
    vTaskDelay(100);
    set_brake(BRAKE_ON);
    vTaskDelay(100);
    CHECK(pdm_brakelight == 0, "short taps don't light it (debounce restarts)");
}

static void sc_r2d_off(void)
{
    vTaskDelay(500);
    CHECK(drive_ready(), "driving");
    set_pedal(300);
    vTaskDelay(100);
    press_r2d();
    CHECK(wait_state(CTRL_STATE_R2D_OFF_WAIT, 100), "R2D button while driving -> R2D_OFF_WAIT");
    vTaskDelay(20);
    CHECK(last_torque == 0, "torque zeroed immediately");
    int t = time_to_state(CTRL_STATE_R2D_WAIT, 800);
    CHECK(t >= 400 && t <= 550, "-> R2D_WAIT after 0.5 s (%d ms)", t);
    vTaskDelay(200);
    CHECK(pdm_pump == 0, "pump off");
    set_pedal(0);
    CHECK(enter_r2d(), "can re-enter R2D");
}

static int max_torque_in_mode(ctrl_mode_t mode)
{
    set_mode(mode);
    vTaskDelay(100);
    if (!drive_ready())
        return -1;
    return torque_after(1000, 200);
}

static void sc_mode_max(void)
{
    vTaskDelay(500);
    int t = max_torque_in_mode(CTRL_MODE_MAX);
    CHECK(t == 2500, "MAX mode: full pedal -> hard max 250.0 Nm (%d)", t);
    CHECK(last_bc_cur_mode == CTRL_MODE_MAX || vcu.ctrl.current_mode == CTRL_MODE_MAX,
          "current mode is MAX");
}

static void sc_mode_crawl(void)
{
    vTaskDelay(500);
    int t = max_torque_in_mode(CTRL_MODE_CRAWL);
    CHECK(t == 200, "CRAWL mode: full pedal -> 20.0 Nm (%d)", t);
    CHECK(last_direction == 0, "forward direction");
}

static void sc_mode_reverse(void)
{
    vTaskDelay(500);
    int t = max_torque_in_mode(CTRL_MODE_REVERSE);
    CHECK(t == 200, "REVERSE mode: full pedal -> 20.0 Nm (%d)", t);
    CHECK(last_direction == 1, "direction bit flipped for reverse");
    int beeps = count_toggles(R2D_SIREN_GPIO_Port, R2D_SIREN_Pin, 2100);
    CHECK(beeps >= 3 && beeps <= 5, "RTDS beeps once a second while reversing (%d edges in 2.1 s)", beeps);
    press_r2d();
    CHECK(wait_state(CTRL_STATE_R2D_OFF_WAIT, 100), "R2D off");
    vTaskDelay(100);
    CHECK(sim_get_output(R2D_SIREN_GPIO_Port, R2D_SIREN_Pin) == 0, "RTDS silent after leaving R2D");
}

static void sc_mode_unknown(void)
{
    vTaskDelay(500);
    set_mode(CTRL_MODE_UNKNOWN_7);
    vTaskDelay(100);
    CHECK(activate_ts(), "TS activates in an undefined mode");
    set_brake(BRAKE_ON);
    press_r2d();
    CHECK(stays_in_state(CTRL_STATE_R2D_WAIT, 2500), "R2D refused in undefined mode 7");
    set_mode(CTRL_MODE_ENDURANCE);
    vTaskDelay(100);
    press_r2d();
    CHECK(wait_state(CTRL_STATE_TS_ON, 3000), "R2D allowed after switching to ENDURANCE");
    set_brake(BRAKE_OFF);
}

static void sc_mode_inverter_prog(void)
{
    vTaskDelay(500);
    set_mode(CTRL_MODE_INVERTER_PROG);
    vTaskDelay(200);
    CHECK(vcu.ctrl.inverter_pwr, "INVERTER_PROG mode powers the inverter without HV");
    CHECK(vcu.ctrl.state == CTRL_STATE_TS_BUTTON_WAIT, "stays in TS_BUTTON_WAIT");
    CHECK(!output_on(TS_ON_GPIO_Port, TS_ON_Pin), "AIRs stay open");
    set_mode(CTRL_MODE_ENDURANCE);
    vTaskDelay(200);
    CHECK(!vcu.ctrl.inverter_pwr, "inverter power off again in ENDURANCE");
}

static void sc_mode_change_while_driving(void)
{
    vTaskDelay(500);
    set_mode(CTRL_MODE_CRAWL);
    vTaskDelay(100);
    CHECK(drive_ready(), "driving in CRAWL");
    set_mode(CTRL_MODE_MAX);
    vTaskDelay(200);
    int t = torque_after(1000, 200);
    CHECK(t == 200, "switching to MAX while driving keeps the CRAWL cap (%d)", t);
    CHECK(vcu.ctrl.current_mode == CTRL_MODE_CRAWL, "current mode still CRAWL");
    CHECK(vcu.ctrl.requested_mode == CTRL_MODE_MAX, "requested mode is MAX");
    set_pedal(0);
    press_r2d();
    CHECK(wait_state(CTRL_STATE_R2D_WAIT, 1000), "R2D off");
    CHECK(enter_r2d(), "R2D again");
    t = torque_after(1000, 200);
    CHECK(t == 2500, "new mode applied on the next R2D (%d)", t);
}

static void sc_apps_bps_plausibility(void)
{
    vTaskDelay(500);
    CHECK(drive_ready(), "driving");
    set_pedal(200);
    set_brake(BRAKE_ON);
    CHECK(stays_in_state(CTRL_STATE_TS_ON, 300), "20%% pedal + brake is allowed (below 30%%)");
    set_pedal(500);
    CHECK(wait_state(CTRL_STATE_APPS_BPS_FAULT, 200), "50%% pedal + brake -> APPS_BPS_FAULT");
    vTaskDelay(30);
    CHECK(last_torque == 0, "torque 0 in the fault (%d)", last_torque);
    set_brake(BRAKE_OFF);
    set_pedal(200);
    CHECK(stays_in_state(CTRL_STATE_APPS_BPS_FAULT, 300), "brake off but pedal still at 20%%: stays in fault");
    set_pedal(0);
    int t = time_to_state(CTRL_STATE_TS_ON, 400);
    CHECK(t >= 80 && t <= 200, "pedal < 5%% for 0.1 s -> back to TS_ON (%d ms)", t);
    CHECK(torque_after(500, 200) > 0, "driving again");
}

static void sc_apps_scs_fault(void)
{
    vTaskDelay(500);
    CHECK(drive_ready(), "driving");
    set_pedal(400);
    vTaskDelay(100);
    sim_adc1[0] = (uint16_t)(sim_adc1[0] + 400); // APPS1 disagrees with APPS2 by ~26%
    CHECK(wait_state(CTRL_STATE_APPS_SCS_FAULT, 100), "APPS sensors disagree -> APPS_SCS_FAULT");
    vTaskDelay(30);
    CHECK(last_torque == 0, "torque 0 (%d)", last_torque);
    vTaskDelay(200);
    CHECK(vcu.tick.apps.scs_error & SCS_ERROR_APPS_DISCREPANCY, "discrepancy flag set");
    set_pedal(0);
    CHECK(wait_state(CTRL_STATE_TS_ON, 100), "recovers when the sensors agree again");
    sim_adc1[0] = 4095; // APPS1 unplugged
    CHECK(wait_state(CTRL_STATE_APPS_SCS_FAULT, 100), "APPS1 unplugged -> APPS_SCS_FAULT");
    sim_adc2[0] = 0; // APPS2 shorted too
    vTaskDelay(100);
    CHECK(vcu.ctrl.state == CTRL_STATE_APPS_SCS_FAULT && last_torque == 0, "still faulted, torque 0");
    CHECK(ack_fault(), "driver can acknowledge with TS -> TS_BUTTON_WAIT");
    vTaskDelay(100);
    if (output_on(TS_ON_GPIO_Port, TS_ON_Pin))
        NOTE("after acknowledging an APPS fault, the AIRs (TS_ON) are still closed in TS_BUTTON_WAIT"
             " while inverter power is switched off - see docs/how-to-run.md (known behaviour)");
    set_pedal(0);
}

static void sc_bps_fault_while_driving(void)
{
    vTaskDelay(500);
    CHECK(drive_ready(), "driving");
    set_pedal(300);
    vTaskDelay(100);
    set_brake(4095); // brake sensor wire broken / shorted to supply
    CHECK(wait_state(CTRL_STATE_APPS_SCS_FAULT, 100), "BPS out of range while driving -> APPS_SCS_FAULT");
    vTaskDelay(30);
    CHECK(last_torque == 0, "torque 0 (%d)", last_torque);
    set_brake(BRAKE_OFF);
    CHECK(wait_state(CTRL_STATE_TS_ON, 100), "recovers when the brake sensor is back in range");
}

static void sc_shutdown_opens_while_driving(void)
{
    vTaskDelay(500);
    CHECK(drive_ready(), "driving");
    set_pedal(500);
    vTaskDelay(100);
    // shutdown circuit opens: the AIRs drop out in hardware, the inverter
    // loses HV and leaves the precharged/running state
    set_shutdown_closed(0);
    inv.vsm = 0;
    inv.dc_voltage = 0;
    int t = 0;
    while (!is_ts_fault() && t < 1000)
    {
        vTaskDelay(1);
        t++;
    }
    CHECK(is_ts_fault() && t <= 50, "TS fault within 50 ms of losing HV (%d ms)", t);
    vTaskDelay(30);
    CHECK(last_torque == 0, "torque 0");
    CHECK(!output_on(TS_ON_GPIO_Port, TS_ON_Pin), "TS_ON output released");
    CHECK(vcu.ctrl.inverter_pwr, "inverter kept powered (motor may still be spinning)");
}

static void sc_inverter_run_fault(void)
{
    vTaskDelay(500);
    CHECK(drive_ready(), "driving");
    set_pedal(400);
    vTaskDelay(100);
    inv.run_lo = 0x0001; // e.g. motor over-speed (run fault LOW word)
    CHECK(wait_ts_fault(100), "PM100 run fault (low word) -> TS fault");
    CHECK(vcu.pm100.error & PM100_ERROR_RUN_FAULT, "run fault latched (0x%x)", vcu.pm100.error);
    CHECK(disable_cmds > 0, "disable command sent to the inverter");
    vTaskDelay(30);
    CHECK(last_torque == 0 && last_enable == 0, "torque 0, inverter disabled");
    inv.run_lo = 0;
    set_pedal(0);
    vTaskDelay(100);
    CHECK(vcu.ctrl.state == CTRL_STATE_TS_RUN_FAULT, "fault stays latched after the inverter clears it");
    CHECK(ack_fault(), "acknowledged");
    vTaskDelay(100);
    CHECK(vcu.pm100.error == 0, "PM100 error cleared");
    CHECK(drive_ready(), "can drive again");
    inv.run_hi = 0x0100; // run fault HIGH word
    CHECK(wait_ts_fault(100), "PM100 run fault (high word) -> TS fault");
}

static void sc_inverter_post_fault(void)
{
    vTaskDelay(500);
    set_shutdown_closed(1);
    inverter_power_up();
    inv.post_hi = 0x0004;
    vTaskDelay(100);
    CHECK(vcu.pm100.error & PM100_ERROR_POST_FAULT, "POST fault latched at power-up");
    press_ts();
    CHECK(wait_state(CTRL_STATE_PRECHARGE_WAIT, 500), "TS activation still starts");
    inv.vsm = 3;
    CHECK(wait_state(CTRL_STATE_R2D_WAIT, 300), "-> R2D_WAIT");
    set_brake(BRAKE_ON);
    press_r2d();
    vTaskDelay(2200);
    set_brake(BRAKE_OFF);
    CHECK(wait_ts_fault(300), "first torque request with a POST fault -> TS fault");
}

static void sc_inverter_lockout(void)
{
    vTaskDelay(500);
    inv.lockout = 1;
    CHECK(drive_ready(), "R2D with the inverter in lockout");
    unsigned d0 = disable_cmds;
    set_pedal(500);
    vTaskDelay(200);
    CHECK(disable_cmds - d0 >= 10, "VCU keeps sending disable commands to clear the lockout (%u)",
          disable_cmds - d0);
    CHECK(last_enable == 0, "no enable command while locked out");
    inv.lockout = 0;
    vTaskDelay(100);
    CHECK(last_enable == 1 && last_torque > 0, "torque sent once the lockout clears (%d)", last_torque);
}

static void sc_inverter_silent_while_driving(void)
{
    vTaskDelay(500);
    CHECK(drive_ready(), "driving");
    set_pedal(300);
    vTaskDelay(100);
    inv.broadcasting = 0; // PM100 stops talking (CAN-T fault, inverter reset)
    int t = 0;
    while (!is_ts_fault() && t < 12000)
    {
        vTaskDelay(10);
        t += 10;
    }
    CHECK(is_ts_fault(), "TS fault when the PM100 stops broadcasting");
    NOTE("detection took %d ms (config pm100.broadcast_timeout_ticks = 10 s)", t);
    CHECK(t >= 9000 && t <= 11000, "detected after the configured 10 s broadcast timeout");
}

static void sc_power_limit(void)
{
    vTaskDelay(500);
    set_mode(CTRL_MODE_MAX);
    vTaskDelay(100);
    CHECK(drive_ready(), "driving in MAX");
    inv.dc_voltage = 500;
    inv.dc_current = 100; // 50 kW
    int t50 = torque_after(1000, 200);
    CHECK(t50 == 2500, "50 kW -> no power derate (%d)", t50);
    inv.dc_current = 152; // 76 kW
    vTaskDelay(100);
    CHECK(last_torque == 2300, "76 kW -> derated by 20.0 Nm (%d)", last_torque);
    inv.dc_current = 160; // 80 kW
    vTaskDelay(100);
    CHECK(last_torque < 1600, "80 kW -> heavily derated (%d)", last_torque);
    inv.dc_current = 50;
    vTaskDelay(100);
    CHECK(last_torque == 2500, "back to full torque below 75 kW (%d)", last_torque);
}

/* ======================================================= launch control */

/*
 * Simple car for the launch control scenarios (1 ms steps): the commanded
 * torque drives the rear wheels through the gearbox, the tyres push the car
 * with a Pacejka-like grip curve (peak at ~14% slip, ~75% of the peak left
 * when spinning), and the front wheels roll at car speed.
 * The PM100 motor speed and all four wheel speed sensors follow the model.
 */
#define CAR_GEAR 3.18
#define CAR_RADIUS 0.20     // rear rolling radius (m), same as config.c
#define CAR_MASS 320.0      // kg with driver
#define CAR_WHEEL_MASS 25.0 // rotating parts (motor, driveline, rear wheels) as mass at the tyre (kg)
#define CAR_REAR_LOAD 0.6   // fraction of the weight on the rear axle
#define CAR_PULSE_M (1.305 / 48.0)
#define CAR_MAX_MOTOR_RPM 6000.0 // the inverter gives no more torque above this

static volatile double car_mu = 0.8;    // tyre grip (0.8: cold / dusty track)
static volatile double car_v, car_w;    // car and rear wheel surface speed (m/s)
static volatile int car_motor_sign = 1; // -1: PM100 reports forward as negative
static volatile int car_running;
static volatile int car_stop_req; // set by car_stop(), done by car_task (no race with a step)

static void car_task(void *arg)
{
    (void)arg;
    TickType_t last = xTaskGetTickCount();
    double front_pulses = 0, rear_pulses = 0;
    for (;;)
    {
        vTaskDelayUntil(&last, 1);
        if (!car_running)
            continue;
        if (car_stop_req)
        {
            car_v = 0;
            car_w = 0;
            car_stop_req = 0;
        }
        const double dt = 0.001;
        double torque_nm = (vcu.ctrl.state == CTRL_STATE_TS_ON && last_torque > 0) ? last_torque / 10.0 : 0.0;
        double v = car_v, w = car_w;
        if (w / CAR_RADIUS * 60.0 / (2.0 * M_PI) * CAR_GEAR >= CAR_MAX_MOTOR_RPM)
            torque_nm = 0.0;
        double drive_n = torque_nm * CAR_GEAR * 0.93 / CAR_RADIUS;
        double slip = (w - v) / (v > 1.0 ? v : 1.0);
        double grip_n = car_mu * CAR_REAR_LOAD * CAR_MASS * 9.81 * sin(1.5 * atan(12.0 * slip));
        w += (drive_n - grip_n) / CAR_WHEEL_MASS * dt;
        v += grip_n / CAR_MASS * dt;
        car_w = (w > 0) ? w : 0;
        car_v = (v > 0) ? v : 0;

        inv.motor_rpm = (int16_t)(car_motor_sign * car_w / CAR_RADIUS * 60.0 / (2.0 * M_PI) * CAR_GEAR);

        front_pulses += car_v * dt / CAR_PULSE_M;
        rear_pulses += car_w * dt / CAR_PULSE_M;
        for (; front_pulses >= 1.0; front_pulses -= 1.0)
        {
            vcu_handle_gpio_exti(&vcu, WHEELSPEED_FL_Pin);
            vcu_handle_gpio_exti(&vcu, WHEELSPEED_FR_Pin);
        }
        for (; rear_pulses >= 1.0; rear_pulses -= 1.0)
        {
            vcu_handle_gpio_exti(&vcu, WHEELSPEED_RL_Pin);
            vcu_handle_gpio_exti(&vcu, WHEELSPEED_RR_Pin);
        }
    }
}

/* bring the car to rest (the model has no brakes): waits for the torque
 * command to reach 0 first, or the wheels spin up again and it creeps on */
static void car_stop(void)
{
    for (int t = 0; t < 200 && last_torque > 0; t++)
        vTaskDelay(1);
    car_stop_req = 1;
    while (car_running && car_stop_req)
        vTaskDelay(1);
}

typedef struct
{
    int ms_to_done;      // time from flooring it to launch control handing back (-1 = never)
    double v_done;       // car speed then (m/s)
    double v_1s;         // car speed 1 s after flooring it (m/s)
    double max_slip_1s;  // worst slip ratio (w - v) / v above 2 m/s in the first second
    double mean_slip;    // mean slip ratio above 2 m/s while launch control was active
    int max_torque;      // highest torque command while active (Nm * 10)
    int min_torque;      // lowest torque command while active
    int first_torque;    // first non-zero torque command after flooring it
    int above_driver;    // torque commands above the 2500 full-pedal request
    float min_limit;     // lowest torque launch control wanted while active
    int any_is_active;   // launch_ctrl_is_active() was true at some point
} launch_result_t;

/* floor it from rest and follow the launch until launch control hands back */
static launch_result_t launch(uint32_t max_ms)
{
    launch_result_t r = { .ms_to_done = -1, .max_torque = 0, .min_torque = 99999, .first_torque = -1,
                          .min_limit = 99999.0f };
    double slip_sum = 0;
    int slip_n = 0;
    const launch_ctrl_context_t *lc = &vcu.ctrl.launch_ctrl;
    const unsigned cmds0 = torque_cmds;

    set_pedal(1000);
    for (uint32_t t = 0; t < max_ms; t++)
    {
        vTaskDelay(1);
        if (r.first_torque < 0 && torque_cmds != cmds0 && last_torque > 0)
            r.first_torque = last_torque;
        double v = car_v, slip = (v > 2.0) ? (car_w - v) / v : 0.0;
        if (t == 1000)
            r.v_1s = v;
        if (t < 1000 && slip > r.max_slip_1s)
            r.max_slip_1s = slip;
        if (lc->state == LAUNCH_CTRL_STATE_ACTIVE)
        {
            if (last_torque > r.max_torque)
                r.max_torque = last_torque;
            if (last_torque < r.min_torque)
                r.min_torque = last_torque;
            if (lc->limit < r.min_limit)
                r.min_limit = lc->limit;
            r.any_is_active |= launch_ctrl_is_active(lc);
            if (v > 2.0)
            {
                slip_sum += slip;
                slip_n++;
            }
        }
        if (last_torque > 2500)
            r.above_driver++;
        if (r.ms_to_done < 0 && t > 50 && lc->state != LAUNCH_CTRL_STATE_ACTIVE)
        {
            r.ms_to_done = (int)t;
            r.v_done = v;
            if (t >= 1000)
                break;
        }
    }
    r.mean_slip = slip_n ? slip_sum / slip_n : 0.0;
    set_pedal(0);
    return r;
}

static void print_launch(const char *name, const launch_result_t *r)
{
    NOTE("%s: first torque %.1f Nm, %.1f m/s after 1 s, worst slip %.0f%% in the first second, "
         "mean slip %.0f%% while active, torque %.1f..%.1f Nm, handed back after %d ms at %.1f m/s",
         name, r->first_torque / 10.0, r->v_1s, r->max_slip_1s * 100, r->mean_slip * 100,
         r->min_torque / 10.0, r->max_torque / 10.0, r->ms_to_done, r->v_done);
}

/* drive in mode with the car model; lc_cfg replaces config.c's launch control settings */
static int launch_setup(const config_launch_ctrl_t *lc_cfg, ctrl_mode_t mode)
{
    vTaskDelay(500);
    if (lc_cfg)
        vcu.ctrl.launch_ctrl_config_ptr = lc_cfg; // picked up on entry to TS_ON
    wheel_period_ms = 0; // the car model drives the wheel speed sensors
    car_stop();
    car_running = 1;
    xTaskCreate(car_task, "carsim", 1024, NULL, 46, NULL);
    set_mode(mode);
    vTaskDelay(100);
    return drive_ready();
}

static config_launch_ctrl_t lc_live_cfg;

static void sc_launch_ctrl(void)
{
    lc_live_cfg = config_get()->launch_ctrl;
    lc_live_cfg.shadow_mode = false;
    const launch_ctrl_context_t *lc = &vcu.ctrl.launch_ctrl;

    CHECK(launch_setup(&lc_live_cfg, CTRL_MODE_TORQUE_CTRL), "driving in TORQUE_CTRL, launch control live (shadow mode off)");
    vTaskDelay(200);
    CHECK(lc->state == LAUNCH_CTRL_STATE_ARMED, "stationary in TORQUE_CTRL -> ARMED (%d)", lc->state);

    launch_result_t r = launch(6000);
    print_launch("launch control live", &r);
    CHECK(r.first_torque >= 1700 && r.first_torque <= 1900,
          "launch starts near torque_start + slip term (%.1f Nm)", r.first_torque / 10.0);
    CHECK(r.min_torque < 2300, "wheelspin -> torque cut below the driver request (min %.1f Nm)",
          r.min_torque / 10.0);
    CHECK(r.min_torque >= lc_live_cfg.torque_floor, "never below torque_floor (%.1f Nm)", r.min_torque / 10.0);
    CHECK(r.max_torque <= lc_live_cfg.torque_max, "never above torque_max (%.1f Nm)", r.max_torque / 10.0);
    CHECK(r.above_driver == 0, "never more than the driver asked for");
    CHECK(r.mean_slip > 0.05 && r.mean_slip < 0.25, "slip held near the 12%% target while active (%.0f%%)",
          r.mean_slip * 100);
    CHECK(r.ms_to_done > 0 && r.v_done > 18.0 && r.v_done < 23.0,
          "hands back at the 20 m/s exit speed (%.1f m/s after %d ms)", r.v_done, r.ms_to_done);
    CHECK(lc->state == LAUNCH_CTRL_STATE_DONE && lc->fault == LAUNCH_CTRL_FAULT_NONE, "DONE, no fault");
    CHECK(vcu.ctrl.state == CTRL_STATE_TS_ON, "still driving");

    // stop, release the pedal: re-arms
    car_stop();
    vTaskDelay(300);
    CHECK(lc->state == LAUNCH_CTRL_STATE_ARMED, "stopped with the pedal released -> ARMED again (%d)", lc->state);

    // lift-off ends a launch
    set_pedal(1000);
    vTaskDelay(300);
    CHECK(lc->state == LAUNCH_CTRL_STATE_ACTIVE, "second launch ACTIVE");
    set_pedal(0);
    vTaskDelay(50);
    CHECK(lc->state == LAUNCH_CTRL_STATE_DONE, "lift-off -> DONE (%d)", lc->state);

    // PM100 motor speed with the wrong sign -> fault -> plain driver torque
    car_stop();
    car_motor_sign = -1;
    vTaskDelay(300);
    CHECK(lc->state == LAUNCH_CTRL_STATE_ARMED, "re-armed (%d)", lc->state);
    set_pedal(1000);
    int faulted = 0;
    for (int t = 0; t < 1000 && !faulted; t++)
    {
        vTaskDelay(1);
        faulted = (lc->state == LAUNCH_CTRL_STATE_FAULT);
    }
    vTaskDelay(30);
    CHECK(faulted && lc->fault == LAUNCH_CTRL_FAULT_MOTOR_REVERSE, "wrong motor speed sign -> MOTOR_REVERSE fault (%d)",
          lc->fault);
    CHECK(last_torque == 2500, "fault -> full driver torque passed through (%d)", last_torque);
    set_pedal(0);
    car_motor_sign = 1;
    car_stop();

    vTaskDelay(11000); // > SD flush period (10 s); run_tests.sh reads the LC lines back from the card
}

static void sc_launch_ctrl_shadow(void)
{
    // config.c as it is (shadow mode on)
    const launch_ctrl_context_t *lc = &vcu.ctrl.launch_ctrl;
    CHECK(config_get()->launch_ctrl.shadow_mode, "config.c has launch control in shadow mode");
    CHECK(launch_setup(NULL, CTRL_MODE_TORQUE_CTRL), "driving in TORQUE_CTRL with config.c");
    vTaskDelay(200);
    CHECK(lc->state == LAUNCH_CTRL_STATE_ARMED, "ARMED (%d)", lc->state);

    launch_result_t r = launch(6000);
    print_launch("shadow mode (no wheelspin control)", &r);
    CHECK(r.first_torque == 2500, "first torque command is the full request (%d)", r.first_torque);
    CHECK(r.min_torque == 2500 && r.max_torque == 2500,
          "shadow mode: the full 250 Nm goes to the inverter for the whole launch (%d..%d)",
          r.min_torque, r.max_torque);
    CHECK(r.min_limit < 2500.0f, "launch control computed a lower limit (down to %.1f Nm)", r.min_limit / 10.0f);
    CHECK(!r.any_is_active, "launch_ctrl_is_active() false in shadow mode");
    CHECK(r.ms_to_done > 0, "launch ends (%d ms)", r.ms_to_done);
    CHECK(r.above_driver == 0, "never more than the driver asked for");
    car_stop();

    vTaskDelay(11000); // > SD flush period (10 s)
}

static void sc_launch_ctrl_only_in_torque_ctrl(void)
{
    // launch control live, but driving in MAX: no launch control at all
    lc_live_cfg = config_get()->launch_ctrl;
    lc_live_cfg.shadow_mode = false;
    const launch_ctrl_context_t *lc = &vcu.ctrl.launch_ctrl;
    CHECK(launch_setup(&lc_live_cfg, CTRL_MODE_MAX), "driving in MAX, launch control live");
    vTaskDelay(200);
    set_pedal(1000);
    int all_full = 1, all_off = 1;
    for (int t = 0; t < 800; t++)
    {
        vTaskDelay(1);
        if (t > 30)
            all_full &= (last_torque == 2500);
        all_off &= (lc->state == LAUNCH_CTRL_STATE_OFF);
    }
    set_pedal(0);
    CHECK(all_off, "launch control stays OFF outside TORQUE_CTRL");
    CHECK(all_full, "full 250 Nm with the wheels spinning (%d)", last_torque);
}

static void sc_fans(void)
{
    vTaskDelay(500);
    inverter_power_up();
    inv.module_temp = 40;
    inv.motor_temp = 30;
    vTaskDelay(300);
    CHECK(pdm_fan == 0, "fan off at 40 C");
    inv.module_temp = 55;
    vTaskDelay(300);
    CHECK(pdm_fan == 1, "fan on above 50 C (inverter module)");
    inv.module_temp = 49;
    vTaskDelay(300);
    CHECK(pdm_fan == 1, "fan stays on at 49 C (hysteresis)");
    inv.module_temp = 40;
    vTaskDelay(300);
    CHECK(pdm_fan == 0, "fan off below 48 C");
    inv.motor_temp = 60;
    vTaskDelay(300);
    CHECK(pdm_fan == 1, "fan on when the motor is hot");
    inv.motor_temp = 30;
    vTaskDelay(300);
    dash_fan_active = 1;
    dash_fan_switch = 0; // config: switch input inverted, 0 = on
    vTaskDelay(400);
    CHECK(pdm_fan == 1 && pdm_pump == 1, "dash fan switch turns fan and pump on");
    dash_fan_switch = 1;
    vTaskDelay(400);
    CHECK(pdm_fan == 0, "dash fan switch off");
}

static void sc_remote_ctrl(void)
{
    vTaskDelay(500);
    set_mode(CTRL_MODE_REMOTE_CTRL);
    set_shutdown_closed(1);
    inverter_power_up();
    remote_active = 1;
    vTaskDelay(300);
    remote_ts_on = 1;
    CHECK(wait_state(CTRL_STATE_WAIT_NEG_AIR, 200), "remote TS on -> TS activation");
    remote_ts_on = 0;
    CHECK(wait_state(CTRL_STATE_PRECHARGE_WAIT, 500), "-> PRECHARGE_WAIT");
    inv.vsm = 3;
    CHECK(wait_state(CTRL_STATE_R2D_WAIT, 300), "-> R2D_WAIT");
    set_brake(BRAKE_OFF);
    remote_r2d = 1;
    CHECK(wait_state(CTRL_STATE_TS_ON, 3000), "remote R2D -> TS_ON (no brake needed)");
    remote_torque = 1500;
    vTaskDelay(200);
    CHECK(last_torque == 1500, "remote torque request followed (%d)", last_torque);
    set_pedal(1000);
    vTaskDelay(100);
    CHECK(last_torque == 1500, "pedal ignored in remote mode (%d)", last_torque);
    set_pedal(0);
    remote_torque = 2400; // above the 230.0 Nm remote limit
    vTaskDelay(100);
    CHECK(last_torque == 0, "request above the remote limit -> 0 (%d)", last_torque);
    remote_torque = 1000;
    vTaskDelay(100);
    CHECK(last_torque == 1000, "back to a valid request (%d)", last_torque);
    remote_active = 0; // remote link lost
    vTaskDelay(1300);
    CHECK(last_torque == 0, "remote link lost for > 1 s -> torque 0 (%d)", last_torque);
}

static void sc_remote_ignored_in_other_modes(void)
{
    vTaskDelay(500);
    set_mode(CTRL_MODE_ENDURANCE);
    set_shutdown_closed(1);
    inverter_power_up();
    remote_active = 1;
    remote_ts_on = 1;
    CHECK(stays_in_state(CTRL_STATE_TS_BUTTON_WAIT, 500), "remote TS ignored in ENDURANCE mode");
}

static void sc_can_flood(void)
{
    vTaskDelay(500);
    CHECK(drive_ready(), "driving");
    set_pedal(500);
    track_gaps = 1;
    max_torque_cmd_gap = 0;
    unsigned n0 = torque_cmds;
    srand(1234);
    for (int i = 0; i < 3000; i++)
    {
        uint8_t data[8];
        for (int b = 0; b < 8; b++)
            data[b] = (uint8_t)rand();
        uint32_t id = (uint32_t)(rand() % 0x7FF);
        if (id == CAN_T_PM100_FAULT_CODES_FRAME_ID || id == CAN_T_PM100_INTERNAL_STATES_FRAME_ID ||
            id == CAN_T_PM100_TEMPERATURE_SET_1_FRAME_ID || id == CAN_T_PM100_TEMPERATURE_SET_3_FRAME_ID ||
            id == CAN_T_PM100_CURRENT_INFO_FRAME_ID || id == CAN_T_PM100_VOLTAGE_INFO_FRAME_ID ||
            id == CAN_T_PM100_MOTOR_POSITION_INFO_FRAME_ID || id == CAN_S_VCU_SIMULATION_FRAME_ID ||
            id == CAN_S_DASH_SENSORS_DIGITAL_FRAME_ID)
            continue; // random contents of known frames would be real faults
        inject(1 + (i & 1), id, data, (uint8_t)(rand() % 9));
        if (i % 10 == 0)
            vTaskDelay(1); // ~10 frames per ms = 10 kframes/s, above a saturated 1 Mbit bus
    }
    vTaskDelay(200);
    CHECK(vcu.ctrl.state == CTRL_STATE_TS_ON, "still driving after 3000 random frames");
    CHECK(torque_cmds - n0 >= 25, "torque commands kept flowing (%u)", torque_cmds - n0);
    CHECK(max_torque_cmd_gap <= 30, "control loop never stalled (max gap %lu ms)",
          (unsigned long)max_torque_cmd_gap);
    unsigned w0 = wheel_rpm_frames;
    vTaskDelay(500);
    CHECK(wheel_rpm_frames - w0 >= 20, "wheel speed broadcasts still running");
    CHECK(last_torque > 0, "still responding to the pedal (%d)", last_torque);
}

static void sc_can_errors(void)
{
    vTaskDelay(500);
    CHECK(drive_ready(), "driving");
    set_pedal(300);
    unsigned l0 = sim_log_lines;
    for (int i = 0; i < 2000; i++)
    {
        hcan1.ErrorCode = HAL_CAN_ERROR_TX_TERR0;
        vcu_handle_can_err(&vcu, &hcan1); // CAN error interrupt
        if (i % 20 == 0)
            vTaskDelay(1);
    }
    hcan1.ErrorCode = 0;
    vTaskDelay(300);
    unsigned lines = sim_log_lines - l0;
    CHECK(lines < 60, "CAN error logging is rate limited (%u log lines for 2000 errors in 0.1 s)", lines);
    CHECK(vcu.ctrl.state == CTRL_STATE_TS_ON, "still driving");
    CHECK(last_torque > 0, "torque still flowing (%d)", last_torque);
}

static void sc_soak(void)
{
    vTaskDelay(500);
    CHECK(drive_ready(), "driving");
    track_gaps = 1;
    max_torque_cmd_gap = 0;
    size_t heap0 = xPortGetFreeHeapSize();
    srand(42);
    int faults = 0;
    TickType_t end = xTaskGetTickCount() + 30000;
    while (xTaskGetTickCount() < end)
    {
        set_pedal(rand() % 1001);
        inv.motor_rpm = (int16_t)(rand() % 6000);
        inv.module_temp = 30 + rand() % 30;
        wheel_period_ms = 2 + rand() % 10;
        vTaskDelay(20 + rand() % 80);
        if (rand() % 100 == 0) // occasional plausibility fault
        {
            set_brake(BRAKE_ON);
            set_pedal(800);
            vTaskDelay(100);
            set_brake(BRAKE_OFF);
            set_pedal(0);
            vTaskDelay(300);
            faults++;
        }
        if (vcu.ctrl.state != CTRL_STATE_TS_ON && vcu.ctrl.state != CTRL_STATE_APPS_BPS_FAULT)
        {
            CHECK(0, "unexpected state %s during soak", sname(vcu.ctrl.state));
            break;
        }
    }
    set_pedal(0);
    vTaskDelay(300);
    CHECK(vcu.ctrl.state == CTRL_STATE_TS_ON, "30 s of random driving + %d plausibility faults: still driving",
          faults);
    CHECK(max_torque_cmd_gap <= 30, "control loop never stalled (max gap %lu ms)",
          (unsigned long)max_torque_cmd_gap);
    CHECK(sim_wdg_expired_at == 0, "watchdog never expired");
    CHECK(xPortGetFreeHeapSize() == heap0, "no FreeRTOS heap leak (%lu -> %lu bytes free)",
          (unsigned long)heap0, (unsigned long)xPortGetFreeHeapSize());
    CHECK(vcu.pm100.error == 0, "no inverter errors");
}

static void sc_init_failure(void)
{
    // vcu_init() was made to fail (sim_fail_task_create) in main()
    vTaskDelay(1500);
    CHECK(vcu.ctrl.thread != NULL && eTaskGetState(vcu.ctrl.thread) == eSuspended,
          "control task suspended after a failed init");
    CHECK(!output_on(TS_ON_GPIO_Port, TS_ON_Pin), "TS stays off");
    set_shutdown_closed(1);
    press_ts();
    vTaskDelay(300);
    CHECK(vcu.ctrl.state == CTRL_STATE_TS_BUTTON_WAIT && !output_on(TS_ON_GPIO_Port, TS_ON_Pin),
          "TS button does nothing");
    int hb = count_toggles(ROB_LED_GPIO_Port, ROB_LED_Pin, 1000);
    CHECK(hb >= 15, "heartbeat LED blinks fast to show the fault (%d toggles in 1 s)", hb);
    CHECK(!sim_wdg_started, "watchdog not started (no reboot loop, the fault stays visible)");
}

static void sc_usb_mode(void)
{
    // USER button held at power-on (set in main)
    vTaskDelay(1000);
    CHECK(sim_usb_init_calls == 1, "USB device started");
    CHECK(vcu.ctrl.thread == NULL, "control task not started in USB mode");
    int hb = count_toggles(ROB_LED_GPIO_Port, ROB_LED_Pin, 1000);
    CHECK(hb >= 15, "heartbeat LED blinks fast in USB mode (%d toggles in 1 s)", hb);
    CHECK(torque_cmds == 0 && vcu_state_frames == 0, "nothing sent on CAN");
    CHECK(!output_on(TS_ON_GPIO_Port, TS_ON_Pin), "TS_ON (AIRs) off in USB mode");
    CHECK(!output_on(STATUS_GPIO_Port, STATUS_Pin), "inverter power output off in USB mode");
    CHECK(!sim_wdg_started, "watchdog not started in USB mode");
}

static void sc_sd_logging(void)
{
    // an SD card image with an existing log_1.txt is attached (set up by run_tests.sh)
    vTaskDelay(1000);
    CHECK(vcu.sd.thread != NULL, "SD logging task started");
    CHECK(strcmp(vcu.sd.log_filename, "log_2.txt") == 0, "next free file chosen: %s", vcu.sd.log_filename);
    CHECK(drive_ready(), "driving");
    set_pedal(400);
    vTaskDelay(100);
    sim_adc1[0] = (uint16_t)(sim_adc1[0] + 400); // APPS discrepancy -> WARN lines
    vTaskDelay(500);
    set_pedal(0);
    vTaskDelay(100);
    for (int i = 0; i < 200; i++)
    {
        LOG_WARN("burst %d\n", i); // 200 warnings in 0.4 s (64-line SD queue)
        vTaskDelay(2);
    }
    vTaskDelay(11000); // > flush period (10 s)
    CHECK(sim_sd_writes > 0, "data written to the card (%u block writes)", sim_sd_writes);
    printf("SDCHECK\n"); // run_tests.sh reads the image back after the process exits
}

static void sc_sd_write_error(void)
{
    // card image attached, but writes fail after a few blocks
    vTaskDelay(1000);
    CHECK(drive_ready(), "driving");
    for (int i = 0; i < 100; i++)
    {
        LOG_WARN("filling %d\n", i);
        vTaskDelay(5);
    }
    vTaskDelay(11000);
    CHECK(vcu.ctrl.state == CTRL_STATE_TS_ON, "SD write failure doesn't affect driving");
    CHECK(torque_after(400, 100) > 0, "torque still flowing");
}

static void sc_sd_absent(void)
{
    vTaskDelay(500);
    CHECK(vcu.sd.thread == NULL, "no SD card -> SD logging not started");
    CHECK(drive_ready(), "car drives normally without an SD card");
}

static void sc_shutdown_opens_during_precharge(void)
{
    vTaskDelay(500);
    set_shutdown_closed(1);
    inverter_power_up(); // never finishes precharge
    press_ts();
    CHECK(wait_state(CTRL_STATE_PRECHARGE_WAIT, 500), "-> PRECHARGE_WAIT");
    vTaskDelay(200);
    set_shutdown_closed(0); // 5 ms glitch
    vTaskDelay(5);
    set_shutdown_closed(1);
    CHECK(stays_in_state(CTRL_STATE_PRECHARGE_WAIT, 200), "a 5 ms glitch on the shutdown input is ignored");
    set_shutdown_closed(0);
    int t = 0;
    while (!is_ts_fault() && t < 4000)
    {
        vTaskDelay(1);
        t++;
    }
    CHECK(is_ts_fault() && t <= 60, "shutdown opening during precharge -> TS fault within 60 ms (%d ms)", t);
    vTaskDelay(30);
    CHECK(!output_on(TS_ON_GPIO_Port, TS_ON_Pin), "AIRs opened");
}

static void sc_shutdown_opens_during_neg_air(void)
{
    vTaskDelay(500);
    set_shutdown_closed(1);
    press_ts();
    CHECK(wait_state(CTRL_STATE_WAIT_NEG_AIR, 50), "-> WAIT_NEG_AIR");
    set_shutdown_closed(0);
    CHECK(wait_ts_fault(60), "shutdown opening while waiting for the AIRs -> TS fault");
}

static void sc_shutdown_opens_during_rtds(void)
{
    vTaskDelay(500);
    CHECK(activate_ts(), "TS active, in R2D_WAIT");
    set_brake(BRAKE_ON);
    press_r2d();
    vTaskDelay(500);
    CHECK(sim_get_output(R2D_SIREN_GPIO_Port, R2D_SIREN_Pin) == 1, "RTDS sounding");
    unsigned st0 = vcu_state_frames;
    vTaskDelay(300);
    CHECK(vcu_state_frames - st0 >= 2, "CAN broadcasts continue while the RTDS sounds");
    set_shutdown_closed(0);
    CHECK(wait_ts_fault(60), "shutdown opening during the RTDS -> TS fault straight away");
    vTaskDelay(30);
    CHECK(sim_get_output(R2D_SIREN_GPIO_Port, R2D_SIREN_Pin) == 0, "RTDS switched off");
    CHECK(!output_on(TS_ON_GPIO_Port, TS_ON_Pin), "AIRs opened");
    vTaskDelay(2500);
    CHECK(is_ts_fault(), "doesn't go to TS_ON when the RTDS time would have ended");
    set_brake(BRAKE_OFF);
}

static void sc_cooling_in_fault(void)
{
    vTaskDelay(500);
    CHECK(drive_ready(), "driving");
    inv.module_temp = 70;
    vTaskDelay(300);
    CHECK(pdm_fan == 1 && pdm_pump == 1, "hot inverter: fan and pump on");
    inv.run_hi = 0x0002; // e.g. inverter over-temperature
    CHECK(wait_ts_fault(100), "inverter fault -> TS fault");
    vTaskDelay(500);
    CHECK(pdm_fan == 1, "fan kept on in the fault while the inverter is hot");
    CHECK(pdm_pump == 1, "pump kept on in the fault while the inverter is hot");
    inv.run_hi = 0;
    CHECK(ack_fault(), "fault acknowledged while still hot");
    vTaskDelay(300);
    CHECK(pdm_pump == 1, "pump still on after the ack while hot");
    inv.module_temp = 30;
    vTaskDelay(500);
    CHECK(pdm_fan == 0 && pdm_pump == 0, "fan and pump off once cooled down");
}

static void sc_log_rate(void)
{
    vTaskDelay(1500);
    unsigned l0 = sim_log_lines_no_ram;
    vTaskDelay(3000);
    unsigned idle = sim_log_lines_no_ram - l0;
    CHECK(idle <= 3, "idle: at most a few log lines in 3 s (%u), no repeated remote timeout", idle);
    CHECK(drive_ready(), "driving");
    set_pedal(500);
    vTaskDelay(200);
    l0 = sim_log_lines_no_ram;
    vTaskDelay(2000);
    CHECK(sim_log_lines_no_ram - l0 <= 4, "driving: no per-cycle logging (%u lines in 2 s)", sim_log_lines_no_ram - l0);
    sim_adc1[0] = (uint16_t)(sim_adc1[0] + 400); // persistent APPS discrepancy
    vTaskDelay(100);
    l0 = sim_log_lines_no_ram;
    vTaskDelay(2000);
    CHECK(sim_log_lines_no_ram - l0 <= 4, "persistent APPS fault: not logged every cycle (%u lines in 2 s)",
          sim_log_lines_no_ram - l0);
    set_pedal(0);
    vTaskDelay(100);
    inv.run_lo = 0x0010; // latched inverter fault
    CHECK(wait_ts_fault(100), "TS fault");
    vTaskDelay(200);
    l0 = sim_log_lines_no_ram;
    vTaskDelay(2000);
    CHECK(sim_log_lines_no_ram - l0 <= 4, "TS fault: not logged every cycle (%u lines in 2 s)", sim_log_lines_no_ram - l0);
    inv.run_lo = 0;
    inv.lockout = 1;
    CHECK(ack_fault(), "acknowledged");
    CHECK(activate_ts() && enter_r2d(), "driving with the inverter in lockout");
    set_pedal(300);
    vTaskDelay(100);
    l0 = sim_log_lines_no_ram;
    vTaskDelay(2000);
    CHECK(sim_log_lines_no_ram - l0 <= 4, "lockout: warned once, not every cycle (%u lines in 2 s)", sim_log_lines_no_ram - l0);
}

static void sc_inverter_power_cycle(void)
{
    vTaskDelay(500);
    CHECK(activate_ts(), "TS active, inverter reports precharge complete");
    press_ts(); // TS off: AIRs open, inverter power off
    CHECK(wait_state(CTRL_STATE_TS_BUTTON_WAIT, 100), "TS off");
    inv.broadcasting = 0; // inverter loses LV power and goes quiet
    inv.vsm = 0;
    vTaskDelay(500);
    press_ts(); // TS on again a moment later
    CHECK(wait_state(CTRL_STATE_PRECHARGE_WAIT, 500), "-> PRECHARGE_WAIT");
    vTaskDelay(300); // inverter still booting: not broadcasting yet
    CHECK(vcu.ctrl.state == CTRL_STATE_PRECHARGE_WAIT,
          "old 'precharged' state from before the power cycle not used");
    inverter_power_up(); // boots, broadcasting VSM start
    vTaskDelay(300);
    CHECK(vcu.ctrl.state == CTRL_STATE_PRECHARGE_WAIT, "waits for the inverter to precharge");
    inv.vsm = 3;
    CHECK(wait_state(CTRL_STATE_R2D_WAIT, 200), "-> R2D_WAIT once it really is precharged");
}

static void sc_watchdog(void)
{
    vTaskDelay(200);
    CHECK(sim_wdg_started, "watchdog started by the control task");
    sim_wdg_reset_gap();
    CHECK(drive_ready(), "TS activation, RTDS and R2D");
    set_pedal(500);
    vTaskDelay(500);
    press_r2d();
    CHECK(wait_state(CTRL_STATE_R2D_WAIT, 1000), "R2D off");
    set_shutdown_closed(0);
    CHECK(wait_ts_fault(100), "TS fault");
    vTaskDelay(300);
    set_shutdown_closed(1);
    CHECK(ack_fault(), "acknowledged");
    CHECK(sim_wdg_max_gap_ms <= 30,
          "kicked every control cycle through activation, RTDS, driving and faults (max gap %u ms)",
          (unsigned)sim_wdg_max_gap_ms);
    CHECK(sim_wdg_expired_at == 0, "never close to expiring");

    // the control loop hangs (e.g. a deadlock): the watchdog must reset the VCU
    TickType_t hang = xTaskGetTickCount();
    vTaskSuspend(vcu.ctrl.thread);
    vTaskDelay(1500);
    CHECK(sim_wdg_expired_at != 0, "control loop stopped -> watchdog resets the VCU");
    if (sim_wdg_expired_at)
        CHECK(sim_wdg_expired_at - hang >= 340 && sim_wdg_expired_at - hang <= 400,
              "after ~0.34 s with the fastest oscillator (0.94 s with the slowest) (%u ms)",
              (unsigned)(sim_wdg_expired_at - hang));
}

static void sc_watchdog_reset_reported(void)
{
    // this boot follows a watchdog reset (VCU_SIM_WATCHDOG_RESET=1)
    vTaskDelay(500);
    CHECK(vcu.ctrl.error & CTRL_ERROR_WATCHDOG_RESET, "watchdog reset flag set in the control errors");
    CHECK(bc_ctrl_error >= 0 && (bc_ctrl_error & CTRL_ERROR_WATCHDOG_RESET),
          "and broadcast in the VCU error CAN frame (0x%x)", bc_ctrl_error);
    CHECK(vcu.ctrl.state == CTRL_STATE_TS_BUTTON_WAIT && !output_on(TS_ON_GPIO_Port, TS_ON_Pin),
          "boots into the safe idle state");
    CHECK(activate_ts(), "car can be started normally");
    CHECK(vcu.ctrl.error & CTRL_ERROR_WATCHDOG_RESET,
          "flag stays set (until a fault is acknowledged or the VCU is powered off)");
}

/* ============================================================ registry */

typedef struct
{
    const char *name;
    void (*fn)(void);
    const char *desc;
} scenario_t;

static const scenario_t scenarios[] = {
    { "boot", sc_boot, "power-up: idle state, heartbeat, CAN broadcasts, wheel speed, outputs safe" },
    { "ts_activation", sc_ts_activation, "TS button -> AIRs -> precharge -> R2D_WAIT timing" },
    { "ts_short_press", sc_ts_short_press, "TS button must be held > 0.5 s and released" },
    { "ts_shutdown_open", sc_ts_shutdown_open, "TS refused while the shutdown circuit is open" },
    { "precharge_timeout", sc_precharge_timeout, "inverter never precharges -> fault after 3 s, ack" },
    { "inverter_silent", sc_inverter_silent, "no inverter on CAN-T -> precharge timeout fault" },
    { "r2d_needs_brake", sc_r2d_needs_brake, "R2D needs brake + long press, RTDS 2 s" },
    { "ts_off_from_r2d_wait", sc_ts_off_from_r2d_wait, "TS button in R2D_WAIT turns TS off" },
    { "shutdown_opens_r2d_wait", sc_shutdown_opens_r2d_wait, "shutdown opens before driving -> fault, ack" },
    { "driving_torque", sc_driving_torque, "torque map, 100 Hz commands, inverter enable logic" },
    { "brake_light", sc_brake_light, "brake light with 0.3 s debounce" },
    { "r2d_off", sc_r2d_off, "R2D button while driving -> R2D_OFF_WAIT -> R2D_WAIT" },
    { "mode_max", sc_mode_max, "MAX mode torque cap" },
    { "mode_crawl", sc_mode_crawl, "CRAWL mode torque cap" },
    { "mode_reverse", sc_mode_reverse, "REVERSE: cap, direction flip, RTDS beeping" },
    { "mode_unknown", sc_mode_unknown, "undefined mode refuses R2D" },
    { "mode_inverter_prog", sc_mode_inverter_prog, "INVERTER_PROG powers the inverter only" },
    { "mode_change_while_driving", sc_mode_change_while_driving, "mode switch only applies at next R2D" },
    { "apps_bps_plausibility", sc_apps_bps_plausibility, "brake + throttle fault and recovery rules" },
    { "apps_scs_fault", sc_apps_scs_fault, "APPS sensor disagreement / unplugged" },
    { "bps_fault_while_driving", sc_bps_fault_while_driving, "brake sensor failure while driving" },
    { "shutdown_opens_while_driving", sc_shutdown_opens_while_driving, "HV lost while driving -> fault" },
    { "inverter_run_fault", sc_inverter_run_fault, "PM100 run faults (low + high word), latch, ack" },
    { "inverter_post_fault", sc_inverter_post_fault, "PM100 POST fault blocks driving" },
    { "inverter_lockout", sc_inverter_lockout, "PM100 enable lockout cleared with disable commands" },
    { "inverter_silent_while_driving", sc_inverter_silent_while_driving, "PM100 stops broadcasting while driving" },
    { "power_limit", sc_power_limit, "75 kW power limiter" },
    { "launch_ctrl", sc_launch_ctrl, "launch control live: wheelspin from rest, exit, lift-off, sign fault" },
    { "launch_ctrl_shadow", sc_launch_ctrl_shadow, "launch control in shadow mode (config.c) never changes torque" },
    { "launch_ctrl_only_in_torque_ctrl", sc_launch_ctrl_only_in_torque_ctrl, "no launch control in other modes" },
    { "fans", sc_fans, "fan thresholds, hysteresis, dash fan switch" },
    { "remote_ctrl", sc_remote_ctrl, "remote control mode: TS/R2D/torque over CAN-S, limit, timeout" },
    { "remote_ignored_in_other_modes", sc_remote_ignored_in_other_modes, "remote ignored outside REMOTE mode" },
    { "can_flood", sc_can_flood, "3000 random CAN frames while driving" },
    { "can_errors", sc_can_errors, "storm of CAN error interrupts while driving" },
    { "soak", sc_soak, "30 s random driving with faults: no stalls, no leaks" },
    { "shutdown_opens_during_neg_air", sc_shutdown_opens_during_neg_air, "shutdown opens while the AIRs close" },
    { "shutdown_opens_during_precharge", sc_shutdown_opens_during_precharge, "shutdown opens during precharge" },
    { "shutdown_opens_during_rtds", sc_shutdown_opens_during_rtds, "control loop keeps running during the RTDS" },
    { "inverter_power_cycle", sc_inverter_power_cycle, "precharge never uses stale inverter state" },
    { "cooling_in_fault", sc_cooling_in_fault, "fan and pump stay on in a fault while hot" },
    { "log_rate", sc_log_rate, "no per-cycle log flooding (idle, driving, faults, lockout)" },
    { "watchdog", sc_watchdog, "watchdog kicked every cycle, resets the VCU if the control loop hangs" },
    { "watchdog_reset_reported", sc_watchdog_reset_reported, "after a watchdog reset: reported, safe idle" },
    { "init_failure", sc_init_failure, "a service fails to start -> car can't be driven" },
    { "usb_mode", sc_usb_mode, "USER button at power-on -> USB mass storage mode" },
    { "sd_logging", sc_sd_logging, "SD card: new log file, WARN lines written, no drops" },
    { "sd_write_error", sc_sd_write_error, "SD card write failure doesn't affect driving" },
    { "sd_absent", sc_sd_absent, "no SD card: logging skipped, car drives" },
};

#define N_SCENARIOS (sizeof(scenarios) / sizeof(scenarios[0]))

static const scenario_t *current;

/* same as vcu_init_task() in src/Core/Src/freertos.c */
static void vcu_init_task(void *arg)
{
    (void)arg;
    status_t st = vcu_init(&vcu, &hcan2, &hcan1, config_get());
    if (strcmp(current->name, "init_failure") == 0)
    {
        if (st == STATUS_OK)
        {
            printf("vcu_init unexpectedly succeeded\n");
            exit(1);
        }
        sim_freertos_init_failure_path(&vcu);
    }
    else if (st != STATUS_OK)
    {
        printf("vcu_init failed\n");
        exit(1);
    }
    vTaskDelete(NULL);
}

static void scenario_task(void *arg)
{
    (void)arg;
    printf("== %s: %s\n", current->name, current->desc);
    current->fn();
    printf("RESULT: %d/%d checks passed\n", checks - failures, checks);
    fflush(stdout);
    exit(failures || checks == 0 ? 1 : 0);
}

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IOLBF, 0);

    if (argc < 2 || strcmp(argv[1], "--list") == 0)
    {
        for (size_t i = 0; i < N_SCENARIOS; i++)
            printf("%-32s %s\n", scenarios[i].name, scenarios[i].desc);
        return argc < 2 ? 2 : 0;
    }
    for (size_t i = 0; i < N_SCENARIOS; i++)
        if (strcmp(argv[1], scenarios[i].name) == 0)
            current = &scenarios[i];
    if (!current)
    {
        fprintf(stderr, "unknown scenario '%s' (try --list)\n", argv[1]);
        return 2;
    }

    // initial inputs: nothing pressed, shutdown open, pedals released
    set_shutdown_closed(0);
    set_pedal(0);
    set_brake(BRAKE_OFF);
    sim_adc1[2] = 100;  // current sensor
    sim_adc1[3] = 2000; // steering
    set_mode(CTRL_MODE_ENDURANCE);

    if (strcmp(current->name, "usb_mode") == 0)
        sim_set_input(USER_BUTTON_GPIO_Port, USER_BUTTON_Pin, 0); // held
    if (strcmp(current->name, "init_failure") == 0)
        sim_fail_task_create = "WHEELSPEED"; // a service after the control task fails

    xTaskCreate(vcu_init_task, "VCU init", 4096, NULL, configMAX_PRIORITIES - 1, NULL);
    xTaskCreate(scenario_task, "scenario", 4096, NULL, 50, NULL);
    xTaskCreate(monitor_task, "monitor", 1024, NULL, 49, NULL);
    xTaskCreate(pm100_task, "pm100sim", 1024, NULL, 45, NULL);
    xTaskCreate(wheel_task, "wheelsim", 1024, NULL, 45, NULL);
    xTaskCreate(remote_task, "remotesim", 1024, NULL, 45, NULL);
    xTaskCreate(dash_fan_task, "dashsim", 1024, NULL, 45, NULL);
    extern void sim_wdg_task(void *arg);
    xTaskCreate(sim_wdg_task, "wdgsim", 1024, NULL, 49, NULL);

    vTaskStartScheduler();
    return 1;
}
