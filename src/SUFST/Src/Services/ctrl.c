/*****************************************************************************
 * @file   driver_control.c
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @author Toby Godfrey (@_tg03, tmag1g21@soton.ac.uk)
 * @brief  Driver control
 ****************************************************************************/

#include "ctrl.h"
#include "clip_to_range.h"

#include <stdbool.h>
#include <stdio.h>

#include "mode_switch_values.h"
#include "rtds.h"
#include "rtos_utils.h"
#include "sd_logging.h"
#include "trc.h"
#include "watchdog.h"

/*
 * internal function prototypes
 */
static void ctrl_thread_entry(void *arg);
static void ctrl_state_machine_tick(ctrl_context_t *ctrl_ptr);
static void ctrl_update_canbc_states(ctrl_context_t *ctrl_ptr);
static uint16_t ctrl_apply_launch_ctrl(ctrl_context_t *ctrl_ptr, uint16_t driver_torque);
static void ctrl_log_launch_ctrl(ctrl_context_t *ctrl_ptr,
                                 launch_ctrl_state_t prev_state,
                                 uint16_t driver_torque);

// the shutdown circuit must read open on this many consecutive control ticks
// (10 ms each) before an activation is aborted, so a single glitch (e.g.
// while the inverter powers up) doesn't abort it
#define CTRL_SHDN_OPEN_TICKS 3U

/**
 * @brief       Initialises control service
 *
 * @param[in]   ctrl_ptr                Control context
 * @param[in]   dash_ptr                Dash context
 * @param[in]   canbc_ptr               CANBC context
 * @param[in]   pm100_ptr               PM100 context
 * @param[in]   tick_ptr                Tick context
 * @param[in]   remote_ctrl_ptr         Remote control context
 * @param[in]   fans_ptr                Fans context
 * @param[in]   wheelspeed_ptr          Wheelspeed context
 * @param[in]   config_ptr              Configuration
 * @param[in]   rtds_config_ptr         RTDS configuration
 * @param[in]   torque_map_config_ptr   Torque map configuration
 * @param[in]   launch_ctrl_config_ptr  Launch control configuration
 */
status_t ctrl_init(ctrl_context_t *ctrl_ptr,
                   dash_context_t *dash_ptr,
                   pm100_context_t *pm100_ptr,
                   tick_context_t *tick_ptr,
                   remote_ctrl_context_t *remote_ctrl_ptr,
                   canbc_context_t *canbc_ptr,
                   fans_context_t *fans_ptr,
                   wheelspeed_context_t *wheelspeed_ptr,
                   const config_ctrl_t *config_ptr,
                   const config_rtds_t *rtds_config_ptr,
                   const config_torque_map_t *torque_map_config_ptr,
                   const config_launch_ctrl_t *launch_ctrl_config_ptr)
{
    ctrl_ptr->state = CTRL_STATE_TS_BUTTON_WAIT;
    ctrl_ptr->prev_state = CTRL_STATE_TS_BUTTON_WAIT;
    ctrl_ptr->rtds_active = false;
    ctrl_ptr->rtds_start = 0;
    ctrl_ptr->shdn_open_count = 0;
    ctrl_ptr->current_mode = CTRL_MODE_ENDURANCE;
    ctrl_ptr->requested_mode = CTRL_MODE_ENDURANCE;
    ctrl_ptr->dash_ptr = dash_ptr;
    ctrl_ptr->pm100_ptr = pm100_ptr;
    ctrl_ptr->tick_ptr = tick_ptr;
    ctrl_ptr->canbc_ptr = canbc_ptr;
    ctrl_ptr->config_ptr = config_ptr;
    ctrl_ptr->rtds_config_ptr = rtds_config_ptr;
    ctrl_ptr->fans_ptr = fans_ptr;
    ctrl_ptr->wheelspeed_ptr = wheelspeed_ptr;
    ctrl_ptr->error = watchdog_caused_last_reset() ? CTRL_ERROR_WATCHDOG_RESET : CTRL_ERROR_NONE;
    ctrl_ptr->apps_reading = 0;
    ctrl_ptr->bps_reading = 0;
    ctrl_ptr->sagl_reading = 0;
    ctrl_ptr->current_reading = 0;
    ctrl_ptr->motor_speed_reading = 0;
    ctrl_ptr->fl_wheel_rpm = 0;
    ctrl_ptr->fr_wheel_rpm = 0;
    ctrl_ptr->wheelspeed_sample_count = 0;
    ctrl_ptr->front_new_sample = false;
    ctrl_ptr->launch_ctrl_config_ptr = launch_ctrl_config_ptr;
    ctrl_ptr->launch_ctrl_last_tick = 0;
    ctrl_ptr->launch_ctrl_log_tick = 0;
    ctrl_ptr->torque_request = 0;
    ctrl_ptr->shdn_reading = 0;
    ctrl_ptr->precharge_start = 0;
    ctrl_ptr->inverter_pwr = false;
    ctrl_ptr->pump_pwr = false;
    ctrl_ptr->remote_ctrl_ptr = remote_ctrl_ptr;

    // initialise the torque map
    status_t status = torque_map_init(&ctrl_ptr->torque_map, torque_map_config_ptr);

    // initialise launch control (and the power limit)
    if (status == STATUS_OK)
    {
        launch_ctrl_init(&ctrl_ptr->launch_ctrl, launch_ctrl_config_ptr);
    }

    // make sure TS is disabled
    trc_set_ts_on(GPIO_PIN_RESET);

    // Set RTDS tick counters to zero
    rtds_pulse_init(&ctrl_ptr->rtds_pulse_ctx, rtds_config_ptr);

    // check all ok before starting the thread (it does not run until the
    // scheduler starts)
    if (status == STATUS_OK)
    {
        status = rtos_task_create(&config_ptr->thread, ctrl_thread_entry, ctrl_ptr,
                                  &ctrl_ptr->thread);
    }

    if (status != STATUS_OK)
    {
        ctrl_ptr->error |= CTRL_ERROR_INIT;
    }

    // send initial state update
    ctrl_update_canbc_states(ctrl_ptr);

    return status;
}

/**
 * @brief       Control thread entry function
 *
 * @param[in]   arg     Control context
 */
static void ctrl_thread_entry(void *arg)
{
    ctrl_context_t *ctrl_ptr = (ctrl_context_t *)arg;

    // from here on the watchdog restarts the VCU if this loop ever stops
    // running for longer than ~0.5 s (see watchdog.c)
    (void)watchdog_start();

    while (1)
    {
        watchdog_kick();

        dash_update_buttons(ctrl_ptr->dash_ptr);

        ctrl_ptr->shdn_reading = trc_ready();

        tick_get_sagl_reading(ctrl_ptr->tick_ptr, &ctrl_ptr->sagl_reading);
        tick_get_current_reading(ctrl_ptr->tick_ptr, &ctrl_ptr->current_reading);

        uint16_t mode_adc_reading;
        if (tick_get_mode_adc_reading(ctrl_ptr->tick_ptr, &mode_adc_reading) == STATUS_OK)
        {
            ctrl_ptr->requested_mode = convert_mode_adc_to_discrete(mode_adc_reading);
        }

        ctrl_ptr->motor_temp = pm100_motor_temp(ctrl_ptr->pm100_ptr);
        ctrl_ptr->inv_temp = pm100_max_inverter_temp(ctrl_ptr->pm100_ptr);
        ctrl_ptr->motor_speed_reading = pm100_motor_speed(ctrl_ptr->pm100_ptr);
        ctrl_ptr->electrical_power_w = pm100_electrical_power_w(ctrl_ptr->pm100_ptr);

        wheelspeed_reading_t wheelspeed_reading;
        ctrl_ptr->front_new_sample = false;
        if (wheelspeed_get_speeds(ctrl_ptr->wheelspeed_ptr, &wheelspeed_reading) == STATUS_OK)
        {
            ctrl_ptr->front_new_sample =
                (wheelspeed_reading.sample_count != ctrl_ptr->wheelspeed_sample_count);
            ctrl_ptr->wheelspeed_sample_count = wheelspeed_reading.sample_count;
            ctrl_ptr->fl_wheel_rpm = wheelspeed_reading.fl_rpm;
            ctrl_ptr->fr_wheel_rpm = wheelspeed_reading.fr_rpm;
        }

        ctrl_ptr->max_temp = ctrl_ptr->motor_temp > ctrl_ptr->inv_temp ?
            ctrl_ptr->motor_temp :
            ctrl_ptr->inv_temp;
        /*
        LOG_INFO("Motor temp: %d   Inverter temp: %d   Max temp: %d\n",

                 ctrl_ptr->motor_temp,
                 ctrl_ptr->inv_temp,
                 ctrl_ptr->max_temp);
        */
        fans_update_thermal(ctrl_ptr->fans_ptr, ctrl_ptr->max_temp);

        ctrl_state_machine_tick(ctrl_ptr);

        bool rtds_pulse_enable = (ctrl_ptr->state == CTRL_STATE_TS_ON) &&
            (ctrl_ptr->current_mode == CTRL_MODE_REVERSE);
        rtds_pulse_tick(&ctrl_ptr->rtds_pulse_ctx, rtds_pulse_enable);

        ctrl_update_canbc_states(ctrl_ptr);

        vTaskDelay(ctrl_ptr->config_ptr->schedule_ticks);
    }
}

/**
 * @brief       Samples the physical dash buttons and remote control overrides for this tick
 *
 * @param[in]   ctrl_ptr        Control context
 * @param[out]  tson_pressed    TS input for this tick (dash button + remote control)
 * @param[out]  r2d_pressed     R2D input for this tick (dash button + remote control)
 */
static void ctrl_sample_driver_inputs(ctrl_context_t *ctrl_ptr, bool *tson_pressed, bool *r2d_pressed)
{
    *tson_pressed = ctrl_ptr->dash_ptr->tson_flag;
    *r2d_pressed = ctrl_ptr->dash_ptr->r2d_flag;

    if (ctrl_ptr->current_mode == CTRL_MODE_REMOTE_CTRL)
    {
        *tson_pressed = *tson_pressed || remote_get_ts_on_pressed(ctrl_ptr->remote_ctrl_ptr);
        *r2d_pressed = *r2d_pressed || remote_get_r2d_pressed(ctrl_ptr->remote_ctrl_ptr);
    }
}

/**
 * @brief       Zeroes the torque request and commands the pm100 to 0
 *
 * @param[in]   ctrl_ptr    Control context
 *
 * @return      False if the pm100 request itself failed (caller should
 *              escalate to CTRL_STATE_TS_RUN_FAULT)
 */
static bool ctrl_fault_zero_torque_ok(ctrl_context_t *ctrl_ptr)
{
    ctrl_ptr->torque_request = 0;
    return pm100_request_torque(ctrl_ptr->pm100_ptr, 0) == STATUS_OK;
}

/**
 * @brief       Re-reads APPS and BPS into ctrl_ptr
 *
 * @param[in]   ctrl_ptr    Control context
 *
 * @return      True if both readings succeeded
 */
static bool ctrl_refresh_apps_bps_ok(ctrl_context_t *ctrl_ptr)
{
    status_t apps_status =
        tick_get_apps_reading(ctrl_ptr->tick_ptr, &ctrl_ptr->apps_reading);
    status_t bps_status = tick_get_bps_reading(ctrl_ptr->tick_ptr, &ctrl_ptr->bps_reading);

    return apps_status == STATUS_OK && bps_status == STATUS_OK;
}

/**
 * @brief       Clears the fault/error state if the driver has pressed TS
 *
 * @param[in]   ctrl_ptr    Control context
 * @param[in]   tson_pressed  TS input for this tick
 *
 * @return      True if the fault was acknowledged and cleared (caller
 *              should return CTRL_STATE_TS_BUTTON_WAIT)
 */
static bool ctrl_try_driver_fault_ack(ctrl_context_t *ctrl_ptr, bool tson_pressed)
{
    if (!tson_pressed)
    {
        return false;
    }

    ctrl_ptr->error = CTRL_ERROR_NONE;
    pm100_clear_error(ctrl_ptr->pm100_ptr);
    tick_clear_apps_scs_error(ctrl_ptr->tick_ptr);

    return true;
}

/**
 * @brief       Debounced check for the shutdown circuit opening during TS
 *              activation (call once per control tick)
 *
 * @return      True once the shutdown circuit has read open for
 *              CTRL_SHDN_OPEN_TICKS consecutive ticks
 */
static bool ctrl_shutdown_open(ctrl_context_t *ctrl_ptr)
{
    if (trc_ready())
    {
        ctrl_ptr->shdn_open_count = 0;
        return false;
    }

    if (ctrl_ptr->shdn_open_count < CTRL_SHDN_OPEN_TICKS)
    {
        ctrl_ptr->shdn_open_count++;
    }

    return ctrl_ptr->shdn_open_count >= CTRL_SHDN_OPEN_TICKS;
}

/**
 * @brief wait for TS button to be held and released
 * then begin activating the TS
 *
 * @param ctrl_ptr
 * @param tson_pressed    TS input for this tick
 * @return ctrl_state_t next state
 */
static ctrl_state_t ctrl_proc_ts_button_wait(ctrl_context_t *ctrl_ptr, bool tson_pressed)
{
    ctrl_ptr->current_mode = ctrl_ptr->requested_mode;
    dash_set_r2d_led_state(ctrl_ptr->dash_ptr, GPIO_PIN_SET);

    // not driving: the pump only runs while cooling is needed
    ctrl_ptr->pump_pwr = ctrl_ptr->fans_ptr->fan_thermal_pwr;

    if (tson_pressed)
    {
        if (trc_ready())
        {
            LOG_INFO("TSON pressed & SHDN closed\n");

            trc_set_ts_on(GPIO_PIN_SET);

            ctrl_ptr->neg_air_start = xTaskGetTickCount();
            return CTRL_STATE_WAIT_NEG_AIR;
        }
    }
    else if (ctrl_ptr->current_mode == CTRL_MODE_INVERTER_PROG)
    {
        ctrl_ptr->inverter_pwr = true;
    }
    else
    {
        // Turn off inverter if TS button is not pressed
        ctrl_ptr->inverter_pwr = false;
    }
    return CTRL_STATE_TS_BUTTON_WAIT;
}

/**
 * @brief Wait for AIRs to close
 *
 * @param ctrl_ptr
 * @return ctrl_state_t next state
 */
static ctrl_state_t ctrl_proc_wait_neg_air(ctrl_context_t *ctrl_ptr)
{
    if (ctrl_shutdown_open(ctrl_ptr))
    {
        LOG_ERROR("SHDN opened during TS activation\n");
        return CTRL_STATE_TS_ACTIVATION_FAILURE;
    }

    if (xTaskGetTickCount() >= ctrl_ptr->neg_air_start + configTICK_RATE_HZ / 4)
    {
        LOG_INFO("Neg AIR closed, turning on inverter\n");
        pm100_forget_state(ctrl_ptr->pm100_ptr);
        ctrl_ptr->inverter_pwr = true;
        ctrl_ptr->precharge_start = xTaskGetTickCount();
        return CTRL_STATE_PRECHARGE_WAIT;
    }
    return CTRL_STATE_WAIT_NEG_AIR;
}

/**
 * @brief TS is ready, can initiate pre-charge sequence
 * TS on LED turns solid
 *
 * @param ctrl_ptr
 * @return ctrl_state_t next state
 */
static ctrl_state_t ctrl_proc_precharge_wait(ctrl_context_t *ctrl_ptr)
{
    if (ctrl_shutdown_open(ctrl_ptr))
    {
        LOG_ERROR("SHDN opened during precharge\n");
        return CTRL_STATE_TS_ACTIVATION_FAILURE;
    }

    const TickType_t charge_time = xTaskGetTickCount() - ctrl_ptr->precharge_start;
    if (pm100_is_precharged(ctrl_ptr->pm100_ptr))
    {
        LOG_INFO("Precharge complete\n");
        return CTRL_STATE_R2D_WAIT;
    }
    else if (charge_time >= ctrl_ptr->config_ptr->precharge_timeout_ticks)
    {
        ctrl_ptr->error |= CTRL_ERROR_PRECHARGE_TIMEOUT;
        LOG_ERROR("Precharge timeout reached\n");
        return CTRL_STATE_TS_ACTIVATION_FAILURE;
    }
    return CTRL_STATE_PRECHARGE_WAIT;
}

/**
 * @brief pre-charge is complete, wait for R2D signal
 * also wait for brake to be fully pressed (if enabled)
 *
 * @param ctrl_ptr
 * @param tson_pressed    TS input for this tick
 * @param r2d_pressed   R2D input for this tick
 * @return ctrl_state_t next state
 */
static ctrl_state_t ctrl_proc_r2d_wait(ctrl_context_t *ctrl_ptr, bool tson_pressed, bool r2d_pressed)
{
    if (ctrl_shutdown_open(ctrl_ptr))
    {
        LOG_ERROR("SHDN opened\n");
        if (ctrl_ptr->rtds_active)
        {
            rtds_set(ctrl_ptr->rtds_config_ptr, false);
            ctrl_ptr->rtds_active = false;
        }
        return CTRL_STATE_TS_ACTIVATION_FAILURE;
    }

    // R2D accepted and the ready-to-drive sound is playing: the control loop
    // keeps running (so the shutdown check above still works) until it ends
    if (ctrl_ptr->rtds_active)
    {
        if ((xTaskGetTickCount() - ctrl_ptr->rtds_start) < ctrl_ptr->rtds_config_ptr->active_ticks)
        {
            return CTRL_STATE_R2D_WAIT;
        }

        rtds_set(ctrl_ptr->rtds_config_ptr, false);
        ctrl_ptr->rtds_active = false;
        ctrl_ptr->apps_bps_start = xTaskGetTickCount();
        LOG_INFO("R2D active\n");
        return CTRL_STATE_TS_ON;
    }

    dash_set_r2d_led_state(ctrl_ptr->dash_ptr, GPIO_PIN_SET);
    ctrl_ptr->current_mode = ctrl_ptr->requested_mode;

    // Stay in R2D_Wait on an unknown mode (e.g. undefined mode or inverter programming)
    if (ctrl_ptr->current_mode == CTRL_MODE_UNKNOWN || ctrl_ptr->current_mode > CTRL_MODE_REMOTE_CTRL)
    {
        return CTRL_STATE_R2D_WAIT;
    }

    if (tson_pressed)
    {
        ctrl_ptr->inverter_pwr = false; // Turn off inverter
        trc_set_ts_on(GPIO_PIN_RESET);  // Turn off AIRs

        return CTRL_STATE_TS_BUTTON_WAIT;
    }

    if (r2d_pressed)
    {
        bool r2d = true;

        if (ctrl_ptr->current_mode != CTRL_MODE_REMOTE_CTRL)
        {
            status_t result =
                tick_get_bps_reading(ctrl_ptr->tick_ptr, &ctrl_ptr->bps_reading);
            if (result != STATUS_OK)
            {
                LOG_ERROR("BPS reading failed\n");
                return CTRL_STATE_TS_ACTIVATION_FAILURE;
            }

            r2d = (ctrl_ptr->config_ptr->r2d_requires_brake) ?
                (ctrl_ptr->bps_reading > ctrl_ptr->config_ptr->bps_on_threshold) :
                1;
        }

        if (r2d)
        {
            dash_set_r2d_led_state(ctrl_ptr->dash_ptr, GPIO_PIN_RESET);
            pm100_disable(ctrl_ptr->pm100_ptr);
            rtds_set(ctrl_ptr->rtds_config_ptr, true);
            ctrl_ptr->rtds_active = true;
            ctrl_ptr->rtds_start = xTaskGetTickCount();
            ctrl_ptr->pump_pwr = 1;

            uint16_t torque_cap = (ctrl_ptr->current_mode == CTRL_MODE_CRAWL ||
                                   ctrl_ptr->current_mode == CTRL_MODE_REVERSE) ?
                ctrl_ptr->config_ptr->crawl_max_torque :
                (ctrl_ptr->current_mode == CTRL_MODE_ENDURANCE) ?
                ctrl_ptr->config_ptr->endurance_max_torque :
                ctrl_ptr->config_ptr->hard_max_torque;

            torque_cap = clip_to_range(torque_cap, 0, ctrl_ptr->config_ptr->hard_max_torque);

            ctrl_ptr->pm100_ptr->reverse_mode_dangerous =
                ctrl_ptr->current_mode == CTRL_MODE_REVERSE;
            if (ctrl_ptr->pm100_ptr->reverse_mode_dangerous)
            {
                LOG_WARN("Reverse active");
            }

            torque_map_set_output_max(&ctrl_ptr->torque_map, torque_cap);
            LOG_INFO("Torque cap (nm x10)%d\n", ctrl_ptr->torque_map.output_max);

            // TS_ON follows once the ready-to-drive sound has finished
            return CTRL_STATE_R2D_WAIT;
        }
    }

    return CTRL_STATE_R2D_WAIT;
}

/**
 * @brief the TS is on
 *
 * @param ctrl_ptr
 * @param r2d_pressed   R2D input for this tick
 * @return ctrl_state_t next state
 */
static ctrl_state_t ctrl_proc_ts_on(ctrl_context_t *ctrl_ptr, bool r2d_pressed)
{
    status_t pm100_status;

    if (r2d_pressed)
    {
        return CTRL_STATE_R2D_OFF;
    }

    if (ctrl_ptr->current_mode != CTRL_MODE_REMOTE_CTRL)
    {
        // read from the APPS
        if (!ctrl_refresh_apps_bps_ok(ctrl_ptr))
        {
            LOG_ERROR("APPS / BPS fault\n");
            return CTRL_STATE_APPS_SCS_FAULT;
        }

        // Check for brake + accel pedal pressed
        if (ctrl_ptr->apps_reading >= ctrl_ptr->config_ptr->apps_bps_high_threshold &&
            ctrl_ptr->bps_reading > ctrl_ptr->config_ptr->apps_bps_fault_bps_threshold)
        {
            LOG_ERROR("BP and AP pressed\n");

            if (xTaskGetTickCount() >= ctrl_ptr->apps_bps_start + (configTICK_RATE_HZ / 50))
            {
                LOG_ERROR("BP-AP fault\n");
                ctrl_ptr->apps_bps_fault_start = xTaskGetTickCount();
                return CTRL_STATE_APPS_BPS_FAULT;
            }
        }
        else
        {
            ctrl_ptr->apps_bps_start = xTaskGetTickCount();
        }

        ctrl_ptr->torque_request =
            torque_map_apply(&ctrl_ptr->torque_map, ctrl_ptr->apps_reading);
    }
    else
    {
        ctrl_ptr->torque_request = remote_get_torque_reading(ctrl_ptr->remote_ctrl_ptr);
    }

    LOG_DEBUG("ADC: %d, Torque: %d\n", ctrl_ptr->apps_reading, ctrl_ptr->torque_request);

    // launch control (LAUNCH_CTRL only) and the power limit (every mode):
    // runs every tick, including at zero request, and only ever lowers the
    // request
    ctrl_ptr->torque_request = ctrl_apply_launch_ctrl(ctrl_ptr, ctrl_ptr->torque_request);

    ctrl_ptr->torque_request =
        clip_to_range(ctrl_ptr->torque_request, 0, ctrl_ptr->config_ptr->hard_max_torque);

    pm100_status = pm100_request_torque(ctrl_ptr->pm100_ptr, ctrl_ptr->torque_request);

    if (pm100_status != STATUS_OK)
    {
        return CTRL_STATE_TS_RUN_FAULT;
    }

    return CTRL_STATE_TS_ON;
}

/**
 * @brief R2D off
 *
 * @param ctrl_ptr
 * @return ctrl_state_t next state
 */
static ctrl_state_t ctrl_proc_r2d_off(ctrl_context_t *ctrl_ptr)
{
    ctrl_ptr->torque_request = 0;
    status_t pm100_status = pm100_request_torque(ctrl_ptr->pm100_ptr, 0);
    ctrl_ptr->motor_torque_zero_start = xTaskGetTickCount();
    ctrl_ptr->pump_pwr = 0;

    if (pm100_status != STATUS_OK)
    {
        return CTRL_STATE_TS_RUN_FAULT;
    }

    return CTRL_STATE_R2D_OFF_WAIT;
}

/**
 * @brief Wait for R2D to turn back on
 *
 * @param ctrl_ptr
 * @return ctrl_state_t next state
 */
static ctrl_state_t ctrl_proc_r2d_off_wait(ctrl_context_t *ctrl_ptr)
{
    dash_set_r2d_led_state(ctrl_ptr->dash_ptr, GPIO_PIN_SET);
    ctrl_ptr->torque_request = 0;
    status_t pm100_status = pm100_request_torque(ctrl_ptr->pm100_ptr, 0);

    if (pm100_status != STATUS_OK)
    {
        return CTRL_STATE_TS_RUN_FAULT;
    }

    if (xTaskGetTickCount() >= ctrl_ptr->motor_torque_zero_start + configTICK_RATE_HZ / 2)
    {
        return CTRL_STATE_R2D_WAIT;
    }

    return CTRL_STATE_R2D_OFF_WAIT;
}

/**
 * @brief SCS fault
 * this is recoverable, if the signal becomes plausible again
 *
 * @param ctrl_ptr
 * @return ctrl_state_t next state
 */
static ctrl_state_t ctrl_proc_apps_scs_fault(ctrl_context_t *ctrl_ptr, bool tson_pressed)
{
    if (!ctrl_fault_zero_torque_ok(ctrl_ptr))
    {
        return CTRL_STATE_TS_RUN_FAULT;
    }

    if (ctrl_refresh_apps_bps_ok(ctrl_ptr))
    {
        return CTRL_STATE_TS_ON;
    }

    if (ctrl_try_driver_fault_ack(ctrl_ptr, tson_pressed))
    {
        return CTRL_STATE_TS_BUTTON_WAIT;
    }

    return CTRL_STATE_APPS_SCS_FAULT;
}

/**
 * @brief
 *
 * @param ctrl_ptr
 * @param tson_pressed    TS input for this tick
 * @return ctrl_state_t next state
 */
static ctrl_state_t ctrl_proc_apps_bps_fault(ctrl_context_t *ctrl_ptr, bool tson_pressed)
{
    if (!ctrl_fault_zero_torque_ok(ctrl_ptr))
    {
        return CTRL_STATE_TS_RUN_FAULT;
    }

    if (ctrl_refresh_apps_bps_ok(ctrl_ptr))
    {
        if ((ctrl_ptr->apps_reading < ctrl_ptr->config_ptr->apps_bps_low_threshold) &&
            ctrl_ptr->bps_reading <= ctrl_ptr->config_ptr->apps_bps_fault_bps_threshold)
        {
            if (xTaskGetTickCount() > ctrl_ptr->apps_bps_fault_start + configTICK_RATE_HZ / 10)
            {
                return CTRL_STATE_TS_ON;
            }
        }
        else
        {
            ctrl_ptr->apps_bps_fault_start = xTaskGetTickCount();
        }
    }

    if (ctrl_try_driver_fault_ack(ctrl_ptr, tson_pressed))
    {
        return CTRL_STATE_TS_BUTTON_WAIT;
    }

    return CTRL_STATE_APPS_BPS_FAULT;
}

/**
 * @brief       Handles an initialisation or runtime fault of the TS and shuts
 *              down the service
 *
 * @param[in]   ctrl_ptr    Control context
 * @param[in]   tson_pressed  TS input for this tick
 */
static ctrl_state_t ctrl_handle_ts_fault(ctrl_context_t *ctrl_ptr, bool tson_pressed)
{

    dash_context_t *dash_ptr = ctrl_ptr->dash_ptr;
    const config_ctrl_t *config_ptr = ctrl_ptr->config_ptr;

    pm100_lvs_off(ctrl_ptr->pm100_ptr);

    // Don't turn off the inverter while the motor could be still spinning (or
    // you might blow up a PCB like stag-11) ctrl_ptr->inverter_pwr = false;

    ctrl_ptr->torque_request = 0;
    pm100_request_torque(ctrl_ptr->pm100_ptr, 0);

    // keep cooling while the inverter or motor is hot (the fans service's
    // thermal hysteresis decides), e.g. after an over-temperature fault
    ctrl_ptr->pump_pwr = ctrl_ptr->fans_ptr->fan_thermal_pwr;

    trc_set_ts_on(GPIO_PIN_RESET);
    dash_blink_ts_on_led(dash_ptr, config_ptr->error_led_toggle_ticks);
    dash_set_r2d_led_state(ctrl_ptr->dash_ptr, GPIO_PIN_SET);

    if (ctrl_try_driver_fault_ack(ctrl_ptr, tson_pressed))
    {
        return CTRL_STATE_TS_BUTTON_WAIT;
    }
    return CTRL_STATE_TS_RUN_FAULT;
}

/**
 * @brief       Runs one tick of the state machine for the control service
 *
 * @param[in]   ctrl_ptr    Control context
 */
static void ctrl_state_machine_tick(ctrl_context_t *ctrl_ptr)
{
    ctrl_state_t next_state = ctrl_ptr->state;
    bool tson_pressed;
    bool r2d_pressed;
    ctrl_sample_driver_inputs(ctrl_ptr, &tson_pressed, &r2d_pressed);

    switch (ctrl_ptr->state)
    {
    case CTRL_STATE_TS_BUTTON_WAIT:
    {
        next_state = ctrl_proc_ts_button_wait(ctrl_ptr, tson_pressed);
        break;
    }
    case CTRL_STATE_WAIT_NEG_AIR:
    {
        next_state = ctrl_proc_wait_neg_air(ctrl_ptr);
        break;
    }
    case CTRL_STATE_PRECHARGE_WAIT:
    {
        next_state = ctrl_proc_precharge_wait(ctrl_ptr);
        break;
    }
    case CTRL_STATE_R2D_WAIT:
    {
        next_state = ctrl_proc_r2d_wait(ctrl_ptr, tson_pressed, r2d_pressed);
        break;
    }
    case CTRL_STATE_TS_ON:
    {
        next_state = ctrl_proc_ts_on(ctrl_ptr, r2d_pressed);
        break;
    }
    case CTRL_STATE_R2D_OFF:
    {
        next_state = ctrl_proc_r2d_off(ctrl_ptr);
        break;
    }
    case CTRL_STATE_R2D_OFF_WAIT:
    {
        next_state = ctrl_proc_r2d_off_wait(ctrl_ptr);
        break;
    }
    // activation or runtime failure
    case CTRL_STATE_TS_ACTIVATION_FAILURE:
    case CTRL_STATE_TS_RUN_FAULT:
    {
        // this state runs every tick, so only log on entry
        if (ctrl_ptr->prev_state != ctrl_ptr->state)
        {
            LOG_ERROR("TS fault during activation or runtime\n");
        }
        next_state = ctrl_handle_ts_fault(ctrl_ptr, tson_pressed);
        break;
    }
    case CTRL_STATE_SPIN:
    {
        // Spin forever
        break;
    }
    case CTRL_STATE_APPS_SCS_FAULT:
    {
        next_state = ctrl_proc_apps_scs_fault(ctrl_ptr, tson_pressed);
        break;
    }
    case CTRL_STATE_APPS_BPS_FAULT:
    {
        next_state = ctrl_proc_apps_bps_fault(ctrl_ptr, tson_pressed);
        break;
    }
    default: break;
    }

    if (next_state == CTRL_STATE_TS_ON && ctrl_ptr->state != CTRL_STATE_TS_ON)
    {
        // reset launch control and the APPS/BPS plausibility timer on every
        // entry to driving, including fault-recovery paths that skip
        // ctrl_proc_r2d_wait
        launch_ctrl_init(&ctrl_ptr->launch_ctrl, ctrl_ptr->launch_ctrl_config_ptr);
        ctrl_ptr->launch_ctrl_last_tick =
            xTaskGetTickCount() - ctrl_ptr->config_ptr->schedule_ticks;
        ctrl_ptr->apps_bps_start = xTaskGetTickCount();
    }

    dash_clear_buttons(ctrl_ptr->dash_ptr);

    ctrl_ptr->prev_state = ctrl_ptr->state;
    ctrl_ptr->state = next_state;
}

/**
 * @brief       Runs one step of launch control on the driver torque request
 *
 * @param[in]   ctrl_ptr        Control context
 * @param[in]   driver_torque   Torque request after the torque map (Nm * 10)
 *
 * @return      Torque request to pass on (never above driver_torque)
 */
static uint16_t ctrl_apply_launch_ctrl(ctrl_context_t *ctrl_ptr, uint16_t driver_torque)
{
    const TickType_t now = xTaskGetTickCount();
    const launch_ctrl_inputs_t inputs = {
        .dt_s = (float)(now - ctrl_ptr->launch_ctrl_last_tick) / (float)configTICK_RATE_HZ,
        .driver_torque = driver_torque,
        .motor_rpm = ctrl_ptr->motor_speed_reading,
        .fl_rpm = ctrl_ptr->fl_wheel_rpm,
        .fr_rpm = ctrl_ptr->fr_wheel_rpm,
        .front_new_sample = ctrl_ptr->front_new_sample,
        .pedal = ctrl_ptr->apps_reading,
        .launch_mode_selected = (ctrl_ptr->current_mode == CTRL_MODE_LAUNCH_CTRL),
        .brake_pressed = (ctrl_ptr->bps_reading > ctrl_ptr->config_ptr->bps_on_threshold),
        .electrical_power_w = ctrl_ptr->electrical_power_w,
    };
    ctrl_ptr->launch_ctrl_last_tick = now;

    const launch_ctrl_state_t prev_state = ctrl_ptr->launch_ctrl.state;
    const uint16_t torque = launch_ctrl_apply(&ctrl_ptr->launch_ctrl, &inputs);

    ctrl_log_launch_ctrl(ctrl_ptr, prev_state, driver_torque);

    return torque;
}

/**
 * @brief       Writes launch control data to the SD card log
 *
 * @details     One line on every state change into or out of ARMED,
 *              ACTIVE or FAULT, and one line per front wheel sample (50 Hz)
 *              while a launch is ACTIVE. Fault changes that don't involve
 *              ACTIVE (faults while armed) are written at most every 100 ms.
 *              OFF <-> DONE changes are not logged. Arming takes a 5 s brake
 *              hold, so driving around in launch mode doesn't fill the log.
 *              Line format:
 *
 *              LC,<tick>,<state>,<fault>,<driver>,<limit>,<output>,<v_ground>,<v_rear>,<v_front>
 *
 *              state / fault: launch_ctrl_state_t / launch_ctrl_fault_t
 *              driver: request into launch control, limit: what it wants,
 *              output: what it passed on (= driver in shadow mode)
 *              v_ground: estimated car speed, v_rear: from motor speed,
 *              v_front: faster front wheel
 *              torques in Nm * 10, speeds in cm/s. The lines go straight to
 *              the SD queue, not over UART, so they cannot hold up the
 *              control loop.
 *
 * @param[in]   ctrl_ptr        Control context
 * @param[in]   prev_state      Launch control state before this step
 * @param[in]   driver_torque   Torque request passed to launch control (Nm * 10)
 */
static void ctrl_log_launch_ctrl(ctrl_context_t *ctrl_ptr,
                                 launch_ctrl_state_t prev_state,
                                 uint16_t driver_torque)
{
    const launch_ctrl_context_t *lc = &ctrl_ptr->launch_ctrl;
    const bool changed = (lc->state != prev_state);
    const bool active_change = changed &&
        (lc->state == LAUNCH_CTRL_STATE_ACTIVE || prev_state == LAUNCH_CTRL_STATE_ACTIVE);
    const bool armed_change = changed &&
        (lc->state == LAUNCH_CTRL_STATE_ARMED || prev_state == LAUNCH_CTRL_STATE_ARMED);
    const bool fault_change = changed &&
        (lc->state == LAUNCH_CTRL_STATE_FAULT || prev_state == LAUNCH_CTRL_STATE_FAULT);
    const bool sample = (lc->state == LAUNCH_CTRL_STATE_ACTIVE) && ctrl_ptr->front_new_sample;

    if (!(active_change || armed_change || sample ||
          (fault_change &&
           rtos_every(&ctrl_ptr->launch_ctrl_log_tick, configTICK_RATE_HZ / 10))))
    {
        return;
    }

    char line[SD_LOG_MSG_SIZE];

    // the C library formatter is not re-entrant (as in log.c)
    taskENTER_CRITICAL();
    snprintf(line, sizeof(line), "LC,%lu,%d,%d,%u,%d,%u,%d,%d,%d\n",
             (unsigned long)xTaskGetTickCount(), (int)lc->state, (int)lc->fault,
             (unsigned)driver_torque, (int)lc->limit, (unsigned)lc->output,
             (int)(lc->v_ground_est * 100.0f), (int)(lc->v_driven * 100.0f),
             (int)(lc->v_front * 100.0f));
    taskEXIT_CRITICAL();

    write_sd_log_line(line);

    if (active_change || armed_change || fault_change)
    {
        LOG_INFO("Launch control state %d -> %d (fault %d)\n", (int)prev_state,
                 (int)lc->state, (int)lc->fault);
    }
}

/**
 * @brief       Updates the CAN broadcast states relevant to the control service
 *
 * @param[in]   ctrl_ptr
 */
static void ctrl_update_canbc_states(ctrl_context_t *ctrl_ptr)
{
    canbc_states_t *states = canbc_lock_state(ctrl_ptr->canbc_ptr, 0);

    if (states != NULL)
    {
        states->state.vcu_r2_d = (ctrl_ptr->state == CTRL_STATE_TS_ON);
        states->sensors.vcu_sagl = ctrl_ptr->sagl_reading;
        states->sensors.vcu_torque_request = ctrl_ptr->torque_request;
        states->temps.vcu_max_temp = ctrl_ptr->max_temp;
        states->state.vcu_ctrl_state = (uint8_t)ctrl_ptr->state;
        states->state.vcu_requested_mode = (uint8_t)ctrl_ptr->requested_mode;
        states->state.vcu_current_mode = (uint8_t)ctrl_ptr->current_mode;
        states->state.vcu_drs_active = ctrl_ptr->shdn_reading;
        states->errors.vcu_ctrl_error = ctrl_ptr->error;
        states->errors.vcu_pm100_error = ctrl_ptr->pm100_ptr->error;
        states->pdm.inverter = ctrl_ptr->inverter_pwr;
        states->pdm.pump = ctrl_ptr->pump_pwr || ctrl_ptr->fans_ptr->fan_switch_status;
        states->pdm.fan = fans_output_pwr(ctrl_ptr->fans_ptr);
        canbc_unlock_state(ctrl_ptr->canbc_ptr);
    }
}
