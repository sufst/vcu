/*****************************************************************************
 * @file   driver_control.c
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief  Driver control
 ****************************************************************************/

#include "ctrl.h"

#include <stdbool.h>

#include "rtds.h"
#include "trc.h"

/*
 * internal function prototypes
 */
void ctrl_thread_entry(ULONG input);
void ctrl_state_machine_tick(ctrl_context_t* ctrl_ptr);
void ctrl_update_canbc_states(ctrl_context_t* ctrl_ptr);
void ctrl_handle_ts_fault(ctrl_context_t* ctrl_ptr);

/**
 * @brief       Initialises control service
 *
 * @param[in]   ctrl_ptr                Control context
 * @param[in]   dash_ptr                Dash context
 * @param[in]   canbc_ptr               CANBC context
 * @param[in]   pm100_ptr               PM100 context
 * @param[in]   stack_pool_ptr          Byte pool to allocate thread stack from
 * @param[in]   config_ptr              Configuration
 * @param[in]   apps_config_ptr         APPS configuration
 * @param[in]   bps_config_ptr          BPS configuration
 * @param[in]   rtds_config_ptr         RTDS configuration
 * @param[in]   torque_map_config_ptr   Torque map configuration
 */
status_t ctrl_init(ctrl_context_t* ctrl_ptr,
                   dash_context_t* dash_ptr,
                   pm100_context_t* pm100_ptr,
                   canbc_context_t* canbc_ptr,
                   TX_BYTE_POOL* stack_pool_ptr,
                   const config_ctrl_t* config_ptr,
                   const config_apps_t* apps_config_ptr,
                   const config_bps_t* bps_config_ptr,
                   const config_rtds_t* rtds_config_ptr,
                   const config_torque_map_t* torque_map_config_ptr)
{
    ctrl_ptr->state = CTRL_STATE_TS_OFF;
    ctrl_ptr->dash_ptr = dash_ptr;
    ctrl_ptr->pm100_ptr = pm100_ptr;
    ctrl_ptr->canbc_ptr = canbc_ptr;
    ctrl_ptr->config_ptr = config_ptr;
    ctrl_ptr->rtds_config_ptr = rtds_config_ptr;
    ctrl_ptr->error = CTRL_ERROR_NONE;
    ctrl_ptr->apps_reading = 0;
    ctrl_ptr->bps_reading = 0;
    ctrl_ptr->sagl_reading = 0;
    ctrl_ptr->torque_request = 0;
    ctrl_ptr->precharge_start = 0;

    // create the thread
    void* stack_ptr = NULL;
    UINT tx_status = tx_byte_allocate(stack_pool_ptr,
                                      &stack_ptr,
                                      config_ptr->thread.stack_size,
                                      TX_NO_WAIT);

    if (tx_status == TX_SUCCESS)
    {
        tx_status = tx_thread_create(&ctrl_ptr->thread,
                                     (CHAR*) config_ptr->thread.name,
                                     ctrl_thread_entry,
                                     (ULONG) ctrl_ptr,
                                     stack_ptr,
                                     config_ptr->thread.stack_size,
                                     config_ptr->thread.priority,
                                     config_ptr->thread.priority,
                                     TX_NO_TIME_SLICE,
                                     TX_AUTO_START);
    }

    status_t status = (tx_status == TX_SUCCESS) ? STATUS_OK : STATUS_ERROR;

    // initialise the APPS, BPS and torque map
    if (status == STATUS_OK)
    {
        status = apps_init(&ctrl_ptr->apps, apps_config_ptr);
    }

    if (status == STATUS_OK)
    {
        status = bps_init(&ctrl_ptr->bps, bps_config_ptr);
    }

    if (status == STATUS_OK)
    {
        status = torque_map_init(&ctrl_ptr->torque_map, torque_map_config_ptr);
    }

    // make sure TS is disabled
    trc_set_ts_on(GPIO_PIN_RESET);

    // check all ok before starting
    if (status != STATUS_OK)
    {
        tx_thread_terminate(&ctrl_ptr->thread);
        ctrl_ptr->error |= CTRL_ERROR_INIT;
    }

    // send initial state update
    ctrl_update_canbc_states(ctrl_ptr);

    return status;
}

/**
 * @brief       Control thread entry function
 *
 * @param[in]   input   Control context
 */
void ctrl_thread_entry(ULONG input)
{
    ctrl_context_t* ctrl_ptr = (ctrl_context_t*) input;

    while (1)
    {
        // uint32_t start_time = tx_time_get();
        ctrl_state_machine_tick(ctrl_ptr);
        ctrl_update_canbc_states(ctrl_ptr);
        // uint32_t run_time = tx_time_get() - start_time;
        // tx_thread_sleep(ctrl_ptr->config_ptr->schedule_ticks - run_time);
    }
}

/**
 * @brief       Runs one tick of the state machine for the control service
 *
 * @param[in]   ctrl_ptr    Control context
 */
void ctrl_state_machine_tick(ctrl_context_t* ctrl_ptr)
{
    // reduce typing...
    dash_context_t* dash_ptr = ctrl_ptr->dash_ptr;
    const config_ctrl_t* config_ptr = ctrl_ptr->config_ptr;

    ctrl_state_t next_state = ctrl_ptr->state;

    switch (ctrl_ptr->state)
    {
    // wait for TS button to be held and released
    // then begin activating the TS
    // LED flashes until activation is complete
    case (CTRL_STATE_TS_OFF):
    {
        dash_wait_for_ts_on(dash_ptr);
        next_state = CTRL_STATE_TS_READY_WAIT;

        break;
    }

    // wait for TS ready from TSAC relay controller
    case (CTRL_STATE_TS_READY_WAIT):
    {
        dash_blink_ts_on_led(dash_ptr, config_ptr->ready_wait_led_toggle_ticks);
        trc_set_ts_on(GPIO_PIN_SET);

        status_t result
            = trc_wait_for_ready(config_ptr->ts_ready_poll_ticks,
                                 config_ptr->ts_ready_timeout_ticks);

        if (result == STATUS_OK)
        {
            tx_thread_sleep(
                5 * TX_TIMER_TICKS_PER_SECOND); // sleep to allow the inrush
                                                // current of AIRs before
                                                // turning on inverter

            next_state = CTRL_STATE_PRECHARGE_WAIT;
            pm100_start_precharge(ctrl_ptr->pm100_ptr);
            ctrl_ptr->precharge_start = tx_time_get();
        }
        else
        {
            ctrl_ptr->error |= CTRL_ERROR_TS_READY_TIMEOUT;
            next_state = CTRL_STATE_TS_ACTIVATION_FAILURE;
        }

        break;
    }

    // TS is ready, can initiate pre-charge sequence
    // TS on LED turns solid
    case (CTRL_STATE_PRECHARGE_WAIT):
    {
        const uint32_t charge_time = tx_time_get() - ctrl_ptr->precharge_start;

        if (pm100_is_precharged(ctrl_ptr->pm100_ptr))
        {
            pm100_disable(ctrl_ptr->pm100_ptr);
            dash_set_ts_on_led_state(dash_ptr, GPIO_PIN_SET);
            next_state = CTRL_STATE_R2D_WAIT;
        }
        else if (charge_time >= config_ptr->precharge_timeout_ticks)
        {
            ctrl_ptr->error |= CTRL_ERROR_PRECHARGE_TIMEOUT;
            next_state = CTRL_STATE_TS_ACTIVATION_FAILURE;
        }

        break;
    }

    // pre-charge is complete, wait for R2D signal
    // also wait for brake to be fully pressed (if enabled)
    case (CTRL_STATE_R2D_WAIT):
    {
        bool r2d = false;
        dash_wait_for_r2d(dash_ptr);

        if (config_ptr->r2d_requires_brake)
        {
            if (bps_fully_pressed(&ctrl_ptr->bps))
            {
                r2d = true;
            }
        }
        else
        {
            r2d = true;
        }

        if (r2d)
        {
            dash_set_r2d_led_state(dash_ptr, GPIO_PIN_SET);
            rtds_activate(ctrl_ptr->rtds_config_ptr);
            next_state = CTRL_STATE_TS_ON;

            // TODO: enable inverter
        }

        break;
    }

    // the TS is on
    case (CTRL_STATE_TS_ON):
    {
        // read from the APPS
        status_t apps_status
            = apps_read(&ctrl_ptr->apps, &ctrl_ptr->apps_reading);

        if (apps_status == STATUS_OK)
        {
            uint16_t torque_request = torque_map_apply(&ctrl_ptr->torque_map,
                                                       ctrl_ptr->apps_reading);
            // TODO: send torque request to inverter
            (void) torque_request;
            __ASM("NOP");
            // status_t pm100_status = pm100_request_torque(ctrl_ptr->pm100_ptr,
            // torque_request);

            // if (pm100_status != STATUS_OK)
            // {
            //     next_state = CTRL_STATE_TS_RUN_FAULT;
            // }
        }
        else
        {
            // TODO: disable inverter
            next_state = CTRL_STATE_APPS_SCS_FAULT;
        }

        // TODO: check for inverter fault, or TRC fault

        break;
    }

    // activation or runtime failure
    case (CTRL_STATE_TS_ACTIVATION_FAILURE):
    case (CTRL_STATE_TS_RUN_FAULT):
    {
        ctrl_handle_ts_fault(ctrl_ptr);
        break;
    }

    // SCS fault
    // this is recoverable, if the signal becomes plausible again
    case (CTRL_STATE_APPS_SCS_FAULT):
    {
        // TODO: request zero torque repeatedly

        if (apps_check_plausibility(&ctrl_ptr->apps))
        {
            next_state = CTRL_STATE_TS_ON;
        }

        break;
    }

    default:
        break;
    }

    ctrl_ptr->state = next_state;
}

/**
 * @brief       Handles an initialisation or runtime fault of the TS and shuts
 *              down the service
 *
 * @param[in]   ctrl_ptr    Control context
 */
void ctrl_handle_ts_fault(ctrl_context_t* ctrl_ptr)
{
    dash_context_t* dash_ptr = ctrl_ptr->dash_ptr;
    const config_ctrl_t* config_ptr = ctrl_ptr->config_ptr;

    pm100_disable(ctrl_ptr->pm100_ptr);
    trc_set_ts_on(GPIO_PIN_RESET);
    dash_blink_ts_on_led(dash_ptr, config_ptr->error_led_toggle_ticks);
    ctrl_update_canbc_states(ctrl_ptr);

    tx_thread_suspend(&ctrl_ptr->thread);
}

/**
 * @brief       Updates the CAN broadcast states relevant to the control service
 *
 * @param[in]   ctrl_ptr
 */
void ctrl_update_canbc_states(ctrl_context_t* ctrl_ptr)
{
    canbc_states_t* states = canbc_lock_state(ctrl_ptr->canbc_ptr, TX_NO_WAIT);

    if (states != NULL)
    {
        // TODO: add ready to drive state?
        states->sensors.vcu_apps = ctrl_ptr->apps_reading;
        states->sensors.vcu_bps = ctrl_ptr->bps_reading;
        states->sensors.vcu_sagl = ctrl_ptr->sagl_reading;
        states->sensors.vcu_torque_request = ctrl_ptr->torque_request;
        states->state.vcu_ctrl_state = (uint8_t) ctrl_ptr->state;
        states->errors.vcu_ctrl_error = ctrl_ptr->error;
        canbc_unlock_state(ctrl_ptr->canbc_ptr);
    }
}