/*****************************************************************************
 * @file   driver_control.c
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief  Driver control
 ****************************************************************************/

#include "ctrl.h"

#include <stdbool.h>

#include "trc.h"

/*
 * internal function prototypes
 */
void ctrl_thread_entry(ULONG input);
void ctrl_state_machine_tick(ctrl_context_t* ctrl_ptr);
void ctrl_update_canbc_states(ctrl_context_t* ctrl_ptr);

/**
 * @brief       Initialises control service
 *
 * @param[in]   ctrl_ptr            Control context
 * @param[in]   dash_ptr            Dash context
 * @param[in]   stack_pool_ptr      Byte pool to allocate thread stack from
 * @param[in]   config_ptr          Configuration
 */
status_t ctrl_init(ctrl_context_t* ctrl_ptr,
                   dash_context_t* dash_ptr,
                   TX_BYTE_POOL* stack_pool_ptr,
                   const config_ctrl_t* config_ptr)
{
    ctrl_ptr->state = CTRL_STATE_TS_OFF;
    ctrl_ptr->dash_ptr = dash_ptr;
    ctrl_ptr->config_ptr = config_ptr;

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

    // make sure TS is disabled
    trc_set_ts_on(GPIO_PIN_RESET);

    return (tx_status == TX_SUCCESS) ? STATUS_OK : STATUS_ERROR;
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
        ctrl_state_machine_tick(ctrl_ptr);
        ctrl_update_canbc_states(ctrl_ptr);
        // TODO: thread sleep??
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

        next_state = (result == STATUS_OK) ? CTRL_STATE_PRECHARGE_WAIT
                                           : CTRL_STATE_TS_ACTIVATION_FAILURE;

        break;
    }

    // TS is ready, can initiate pre-charge sequence
    // TS on LED turns solid
    case (CTRL_STATE_PRECHARGE_WAIT):
    {
        // TODO: wait for pre-charge from PM100

        dash_set_ts_on_led_state(dash_ptr, GPIO_PIN_SET);
        next_state = CTRL_STATE_R2D_WAIT;
        break;
    }

    // pre-charge is complete, wait for R2D signal
    case (CTRL_STATE_R2D_WAIT):
    {
        dash_wait_for_r2d(dash_ptr);
        dash_set_r2d_led_state(dash_ptr, GPIO_PIN_SET);

        // TODO: enable inverter

        next_state = CTRL_STATE_TS_ON;
        break;
    }

    // the TS is on
    case (CTRL_STATE_TS_ON):
    {
        // TODO: read APPS
        // TODO: send torque request to PM100
        break;
    }

    // activation failure
    // LED blinks (should be set faster in config)
    case (CTRL_STATE_TS_ACTIVATION_FAILURE):
    {
        trc_set_ts_on(GPIO_PIN_RESET);
        dash_blink_ts_on_led(dash_ptr, config_ptr->error_led_toggle_ticks);

        // TODO: update broadcast states before suspending
        tx_thread_suspend(&ctrl_ptr->thread);
        break;
    }

    default:
        break;
    }

    ctrl_ptr->state = next_state;
}

/**
 * @brief       Updates the CAN broadcast states relevant to the control service
 *
 * @param[in]   ctrl_ptr
 */
void ctrl_update_canbc_states(ctrl_context_t* ctrl_ptr)
{
    // TODO: implement
}