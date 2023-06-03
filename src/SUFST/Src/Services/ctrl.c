/*****************************************************************************
 * @file   driver_control.c
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief  Driver control
 ****************************************************************************/

#include "ctrl.h"

#include <stdbool.h>

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
 * @param[in]   thread_config_ptr   Thread configuration
 * @param[in]   ts_activation_config_ptr    TS activation configuration
 */
status_t ctrl_init(ctrl_context_t* ctrl_ptr,
                   dash_context_t* dash_ptr,
                   TX_BYTE_POOL* stack_pool_ptr,
                   const config_thread_t* thread_config_ptr,
                   const config_ts_activation_t* ts_activation_config_ptr)
{
    ctrl_ptr->state = CTRL_STATE_TS_OFF;
    ctrl_ptr->dash_ptr = dash_ptr;
    ctrl_ptr->ts_activation_config_ptr = ts_activation_config_ptr;

    // create the thread
    void* stack_ptr = NULL;
    UINT tx_status = tx_byte_allocate(stack_pool_ptr,
                                      &stack_ptr,
                                      thread_config_ptr->stack_size,
                                      TX_NO_WAIT);

    if (tx_status == TX_SUCCESS)
    {
        tx_status = tx_thread_create(&ctrl_ptr->thread,
                                     (CHAR*) thread_config_ptr->name,
                                     ctrl_thread_entry,
                                     (ULONG) ctrl_ptr,
                                     stack_ptr,
                                     thread_config_ptr->stack_size,
                                     thread_config_ptr->priority,
                                     thread_config_ptr->priority,
                                     TX_NO_TIME_SLICE,
                                     TX_AUTO_START);
    }

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
    }
}

/**
 * @brief       Runs one tick of the state machine for the control service
 *
 * @param[in]   ctrl_ptr    Control context
 */
void ctrl_state_machine_tick(ctrl_context_t* ctrl_ptr)
{
    ctrl_state_t next_state = ctrl_ptr->state;
    dash_context_t* dash_ptr = ctrl_ptr->dash_ptr;

    switch (ctrl_ptr->state)
    {
    // wait for TS button to be held and released
    case (CTRL_STATE_TS_OFF):
    {
        dash_wait_for_ts_on(dash_ptr);

        dash_blink_ts_on_led(
            dash_ptr,
            ctrl_ptr->ts_activation_config_ptr->ready_wait_led_toggle_ticks);

        next_state = CTRL_STATE_TS_READY_WAIT;

        break;
    }

    // wait for TS ready from TSAC relay controller
    case (CTRL_STATE_TS_READY_WAIT):
    {
        // TODO: wait for TRC instead of sleep
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND * 2);

        dash_set_ts_on_led_state(dash_ptr, GPIO_PIN_SET);
        next_state = CTRL_STATE_PRECHARGE_WAIT;
        break;
    }

    // TS is ready, can initiate pre-charge sequence
    case (CTRL_STATE_PRECHARGE_WAIT):
    {
        // TODO: wait for pre-charge from PM100
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
        break;
    }

    // activation failure
    case (CTRL_STATE_TS_ACTIVATION_FAILURE):
    {
        dash_blink_ts_on_led(
            dash_ptr,
            ctrl_ptr->ts_activation_config_ptr->error_led_toggle_ticks);

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

// /*
//  * internal functions
//  */
// static void driver_ctrl_thread_entry(ULONG input);
// static void driver_ctrl_tick_callback(ULONG input);
// static void do_activation_logic(const config_ts_activation_t* config_ptr);
// static bool get_ts_ready_state();
// static bool input_wait(bool state, input_context_t* input_ptr);
// static void update_canbc_states(dc_handle_t* dc_h);
// static bool no_errors(dc_handle_t* dc_h);
// static driver_ctrl_status_t create_status(dc_handle_t* dc_h);

// /**
//  * @brief       Initialises driver controls
//  *
//  * @param[in]   dc_h                        Driver control handle
//  * @param[in]   ts_ctrl_h                   TS controller handle
//  * @param[in]   canbc_h                     CANBC service handle
//  * @param[in]   stack_pool_ptr              Pool to allocate stack memory
//  from
//  * @param[in]   ts_activation_config_ptr    Pointer to config for TS
//  activation
//  */
// driver_ctrl_status_t
// driver_ctrl_init(dc_handle_t* dc_h,
//                  ts_ctrl_handle_t* ts_ctrl_h,
//                  canbc_handle_t* canbc_h,
//                  TX_BYTE_POOL* stack_pool_ptr,
//                  const config_ts_activation_t* ts_activation_config_ptr)
// {
//     dc_h->ts_ctrl_h = ts_ctrl_h;
//     dc_h->canbc_h = canbc_h;
//     dc_h->ts_activation_config_ptr = ts_activation_config_ptr;

//     dc_h->ts_inputs.accel_pressure = 0;
//     dc_h->ts_inputs.brake_pressure = 0;

//     update_canbc_states(dc_h);

//     // thread
//     {
//         void* stack_ptr = NULL;

//         UINT status = tx_byte_allocate(stack_pool_ptr,
//                                        &stack_ptr,
//                                        DRIVER_CTRL_THREAD_STACK_SIZE,
//                                        TX_NO_WAIT);

//         if (status == TX_SUCCESS)
//         {
//             status = tx_thread_create(&dc_h->thread,
//                                       DRIVER_CTRL_THREAD_NAME,
//                                       driver_ctrl_thread_entry,
//                                       (ULONG) dc_h,
//                                       stack_ptr,
//                                       DRIVER_CTRL_THREAD_STACK_SIZE,
//                                       DRIVER_CTRL_THREAD_PRIORITY,
//                                       DRIVER_CTRL_THREAD_PRIORITY,
//                                       TX_NO_TIME_SLICE,
//                                       TX_DONT_START);
//         }

//         if (status != TX_SUCCESS)
//         {
//             dc_h->err |= DRIVER_CTRL_ERROR_INIT;
//         }
//     }

//     // can broadcast service
//     if (no_errors(dc_h))
//     {
//         UINT status = tx_mutex_create(&dc_h->state_mutex,
//                                       DRIVER_CTRL_STATE_MUTEX_NAME,
//                                       TX_INHERIT);

//         if (status != TX_SUCCESS)
//         {
//             dc_h->err |= DRIVER_CTRL_ERROR_INIT;
//         }
//     }

//     return create_status(dc_h);
// }

// /**
//  * @brief       Start reading driver controls
//  *
//  * @param[in]   dc_h   Driver control handle
//  */
// driver_ctrl_status_t driver_ctrl_start(dc_handle_t* dc_h)
// {
//     UINT status = tx_thread_resume(&dc_h->thread);

//     if (status != TX_SUCCESS)
//     {
//         dc_h->err |= DRIVER_CTRL_ERROR_INTERNAL;
//     }

//     return create_status(dc_h);
// }

// /**
//  * @brief       Tick timer callback
//  *
//  * @param[in]   input   Driver control handle
//  */
// static void driver_ctrl_tick_callback(ULONG input)
// {
//     dc_handle_t* dc_h = (dc_handle_t*) input;

//     UINT status = tx_thread_resume(&dc_h->thread);

//     if (status != TX_SUCCESS)
//     {
//         dc_h->err |= DRIVER_CTRL_ERROR_INTERNAL;
//     }
// }

// /**
//  * @brief       Driver control thread entry function
//  *
//  * @param[in]   input   Driver control handle
//  */
// static void driver_ctrl_thread_entry(ULONG input)
// {
//     dc_handle_t* dc_h = (dc_handle_t*) input;

//     // activation logic
//     do_activation_logic(dc_h->ts_activation_config_ptr);

//     // create tick timer
//     const ULONG ticks = TX_TIMER_TICKS_PER_SECOND / DRIVER_CTRL_TICK_RATE;

//     UINT status = tx_timer_create(&dc_h->tick_timer,
//                                   DRIVER_CTRL_TIMER_NAME,
//                                   driver_ctrl_tick_callback,
//                                   (ULONG) dc_h,
//                                   ticks,
//                                   ticks,
//                                   TX_NO_ACTIVATE);

//     // sample driver inputs
//     if (status == TX_SUCCESS)
//     {
//         while (1)
//         {
//             // read inputs
// #if RUN_APPS_TESTBENCH
//             dc_h->ts_inputs.accel_pressure = testbench_apps_input();
// #else
//             dc_h->ts_inputs.accel_pressure = (ULONG) apps_read();
// #endif

//             dc_h->ts_inputs.brake_pressure = bps_read();

//             // send inputs to TS controller
//             if (ts_ctrl_input_send(dc_h->ts_ctrl_h, &dc_h->ts_inputs)
//                 != TS_CTRL_OK)
//             {
//                 // TODO: handle error?
//                 Error_Handler();
//             }

//             // update states all at the same time
//             update_canbc_states(dc_h);

//             // sleep thread to allow other threads to run
//             // TODO: should the timer activation happen in the timer
//             callback??
//             //       that would give potentially more stable ticks
//             tx_timer_activate(&dc_h->tick_timer);
//             tx_thread_suspend(&dc_h->thread);
//         }
//     }
//     else
//     {
//         dc_h->err |= DRIVER_CTRL_ERROR_INTERNAL;
//         Error_Handler();
//     }
// }

// /**
//  * @brief       Completes the TS activation procedure
//  *
//  * @details     This is implemented as a simple state machine
//  *
//  * @param[in]   config_ptr  Pointer to TS activation configuration
//  */
// void do_activation_logic(const config_ts_activation_t* config_ptr)
// {
//     enum
//     {
//         STATE_TS_OFF,
//         STATE_READY_WAIT,
//         STATE_PRECHARGE_WAIT,
//         STATE_READY,
//         STATE_TS_ON,
//         STATE_ACTIVATION_FAILURE
//     } state,
//         next_state;

//     state = STATE_TS_OFF;
//     next_state = state;

//     uint32_t last_led_toggle = tx_time_get();
//     uint32_t ts_ready_wait_start_time = 0;
//     uint32_t precharge_start_time = 0;

//     // inputs must be held and released to avoid accidental activation
//     input_context_t ts_on_input
//         = {.active_state = true,
//            .last_state = false,
//            .require_release = true,
//            .active_start = 0,
//            .required_ticks = config_ptr->input_active_ticks};

//     input_context_t ts_ready_input
//         = {.active_state = true,
//            .last_state = false,
//            .require_release = false,
//            .active_start = 0,
//            .required_ticks = config_ptr->input_active_ticks};

//     input_context_t r2d_input
//         = {.active_state = true,
//            .last_state = false,
//            .require_release = true,
//            .required_ticks = config_ptr->input_active_ticks,
//            .active_start = 0};

//     // system controller state machine
//     while (true)
//     {
//         switch (state)
//         {
//         // wait for TS on button to be held and released
//         case (STATE_TS_OFF):
//         {
//             if (input_wait(dash_get_ts_on_btn_state(), &ts_on_input))
//             {
//                 next_state = STATE_READY_WAIT;
//                 ts_ready_wait_start_time = tx_time_get();
//             }
//             break;
//         }

//         // wait for TS ready from TSAC relay controller
//         // flash the TS LED, time out if it takes too long
//         case (STATE_READY_WAIT):
//         {
//             // flash LED while waiting
//             uint32_t time = tx_time_get();

//             if ((time - last_led_toggle)
//                 >= config_ptr->ready_wait_led_toggle_ticks)
//             {
//                 last_led_toggle = time;
//                 dash_toggle_ts_on_led();
//             }

//             // monitor input until ready or timeout
//             if (input_wait(get_ts_ready_state(), &ts_ready_input))
//             {
//                 // TODO: send precharge command to inverter
//                 precharge_start_time = tx_time_get();
//                 next_state = STATE_PRECHARGE_WAIT;
//             }
//             else if ((time - ts_ready_wait_start_time)
//                      > config_ptr->ts_ready_timeout_ticks)
//             {
//                 next_state = STATE_ACTIVATION_FAILURE;
//             }

//             break;
//         }

//         // the relay controller is ready
//         // this requires that the BMS is OK and the inverter is not faulted
//         // precharge sequence can begin
//         case (STATE_PRECHARGE_WAIT):
//         {
//             // flash LED while waiting
//             // TODO: code duplication?
//             uint32_t time = tx_time_get();

//             if ((time - last_led_toggle)
//                 >= config_ptr->ready_wait_led_toggle_ticks)
//             {
//                 last_led_toggle = time;
//                 dash_toggle_ts_on_led();
//             }

//             // wait for precharge to complete or timeout
//             if (true) // TODO: precharge from state pm100 module
//             {
//                 next_state = STATE_READY;
//             }
//             else if ((time - precharge_start_time)
//                      > config_ptr->precharge_timeout_ticks)
//             {
//                 next_state = STATE_ACTIVATION_FAILURE;
//             }

//             break;
//         }

//         // precharge is complete
//         // driver can press R2D to activate inverter output (exit lockout)
//         case (STATE_READY):
//         {
//             if (input_wait(dash_get_r2d_btn_state(), &r2d_input))
//             {
//                 next_state = STATE_TS_ON;
//                 dash_set_r2d_led_state(GPIO_PIN_SET);
//             }

//             break;
//         };

//         // 4. The TS is on, activation is complete
//         case (STATE_TS_ON):
//         {
//             // TODO: integrate sensor reading code here
//             break;
//         }

//         case (STATE_ACTIVATION_FAILURE):
//         {
//             // flash LED while waiting
//             uint32_t time = tx_time_get();

//             if ((time - last_led_toggle) >=
//             config_ptr->error_led_toggle_ticks)
//             {
//                 last_led_toggle = time;
//                 dash_toggle_ts_on_led();
//             }
//             break;
//         }

//         default:
//             break;
//         };

//         state = next_state;
//         tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / DRIVER_CTRL_TICK_RATE);
//     }

//     // TODO: activate TS ON output and LED
//     // TODO: start TS control service

//     // TODO: wait for TS ready
//     // TODO: optional reading from BPS
//     // TODO: update broadcast states
//     // TODO: ? deactivation logic
// }

// /**
//  * @brief   Returns the state of the TS ready input
//  */
// bool get_ts_ready_state()
// {
//     return (HAL_GPIO_ReadPin(TS_READY_GPIO_Port, TS_READY_Pin) ==
//     GPIO_PIN_SET);
// }

// /**
//  * @brief       Update CANBC states
//  *
//  * @param[in]   dc_h   Driver control handle
//  */
// static void update_canbc_states(dc_handle_t* dc_h)
// {
//     canbc_handle_t* canbc_h = dc_h->canbc_h;
//     canbc_states_t* state_ptr = canbc_lock_state(canbc_h, TX_NO_WAIT);

//     if (state_ptr != NULL)
//     {
//         state_ptr->apps_reading = dc_h->ts_inputs.accel_pressure;
//         state_ptr->bps_reading = dc_h->ts_inputs.brake_pressure;

//         canbc_unlock_state(canbc_h);
//     }
// }

// /**
//  * @brief       Returns true if the driver control instance has encountered
//  an
//  *              error
//  *
//  * @param[in]   dc_h   Driver control handle
//  */
// static bool no_errors(dc_handle_t* dc_h)
// {
//     return (dc_h->err == DRIVER_CTRL_ERROR_NONE);
// }

// /**
//  * @brief       Create a status code based on the current error state
//  *
//  * @param[in]   dc_h   Driver control handle
//  */
// static driver_ctrl_status_t create_status(dc_handle_t* dc_h)
// {
//     return (no_errors(dc_h)) ? DRIVER_CTRL_OK : DRIVER_CTRL_ERROR;
// }