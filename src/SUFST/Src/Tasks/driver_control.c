/*****************************************************************************
 * @file   driver_control.c
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief  Driver control
 ****************************************************************************/

#include "driver_control.h"

#include <stdbool.h>

#include "Test/testbench.h"
#include "apps.h"
#include "bps.h"
#include "config.h"

/*
 * macro constants
 */
#define DRIVER_CTRL_THREAD_STACK_SIZE 1024
#define DRIVER_CTRL_THREAD_NAME       "Driver Control Thread"
#define DRIVER_CTRL_TIMER_NAME        "Driver Control Tick Timer"
#define DRIVER_CTRL_STATE_MUTEX_NAME  "Driver Control State Mutex"

/*
 * internal functions
 */
static void driver_ctrl_thread_entry(ULONG input);
static void driver_ctrl_tick_callback(ULONG input);
static void do_activation_logic(dc_handle_t* dc_h);
static void update_canbc_states(dc_handle_t* dc_h);
static bool no_errors(dc_handle_t* dc_h);
static driver_ctrl_status_t create_status(dc_handle_t* dc_h);

/**
 * @brief       Initialises driver controls
 *
 * @param[in]   dc_h   Driver control handle
 * @param[in]   ts_ctrl_h       TS controller handle
 * @param[in]   canbc_h         CANBC service handle
 * @param[in]   stack_pool_ptr  Memory pool to allocate stack memory from
 */
driver_ctrl_status_t driver_ctrl_init(dc_handle_t* dc_h,
                                      ts_ctrl_handle_t* ts_ctrl_h,
                                      canbc_handle_t* canbc_h,
                                      TX_BYTE_POOL* stack_pool_ptr)
{
    dc_h->ts_ctrl_h = ts_ctrl_h;
    dc_h->canbc_h = canbc_h;

    dc_h->ts_inputs.accel_pressure = 0;
    dc_h->ts_inputs.brake_pressure = 0;

    update_canbc_states(dc_h);

    // thread
    {
        void* stack_ptr = NULL;

        UINT status = tx_byte_allocate(stack_pool_ptr,
                                       &stack_ptr,
                                       DRIVER_CTRL_THREAD_STACK_SIZE,
                                       TX_NO_WAIT);

        if (status == TX_SUCCESS)
        {
            status = tx_thread_create(&dc_h->thread,
                                      DRIVER_CTRL_THREAD_NAME,
                                      driver_ctrl_thread_entry,
                                      (ULONG) dc_h,
                                      stack_ptr,
                                      DRIVER_CTRL_THREAD_STACK_SIZE,
                                      DRIVER_CTRL_THREAD_PRIORITY,
                                      DRIVER_CTRL_THREAD_PRIORITY,
                                      TX_NO_TIME_SLICE,
                                      TX_DONT_START);
        }

        if (status != TX_SUCCESS)
        {
            dc_h->err |= DRIVER_CTRL_ERROR_INIT;
        }
    }

    // can broadcast service
    if (no_errors(dc_h))
    {
        UINT status = tx_mutex_create(&dc_h->state_mutex,
                                      DRIVER_CTRL_STATE_MUTEX_NAME,
                                      TX_INHERIT);

        if (status != TX_SUCCESS)
        {
            dc_h->err |= DRIVER_CTRL_ERROR_INIT;
        }
    }

    return create_status(dc_h);
}

/**
 * @brief       Start reading driver controls
 *
 * @param[in]   dc_h   Driver control handle
 */
driver_ctrl_status_t driver_ctrl_start(dc_handle_t* dc_h)
{
    UINT status = tx_thread_resume(&dc_h->thread);

    if (status != TX_SUCCESS)
    {
        dc_h->err |= DRIVER_CTRL_ERROR_INTERNAL;
    }

    return create_status(dc_h);
}

/**
 * @brief       Tick timer callback
 *
 * @param[in]   input   Driver control handle
 */
static void driver_ctrl_tick_callback(ULONG input)
{
    dc_handle_t* dc_h = (dc_handle_t*) input;

    UINT status = tx_thread_resume(&dc_h->thread);

    if (status != TX_SUCCESS)
    {
        dc_h->err |= DRIVER_CTRL_ERROR_INTERNAL;
    }
}

/**
 * @brief       Driver control thread entry function
 *
 * @param[in]   input   Driver control handle
 */
static void driver_ctrl_thread_entry(ULONG input)
{
    dc_handle_t* dc_h = (dc_handle_t*) input;

    // create tick timer
    const ULONG ticks = TX_TIMER_TICKS_PER_SECOND / DRIVER_CTRL_TICK_RATE;

    UINT status = tx_timer_create(&dc_h->tick_timer,
                                  DRIVER_CTRL_TIMER_NAME,
                                  driver_ctrl_tick_callback,
                                  (ULONG) dc_h,
                                  ticks,
                                  ticks,
                                  TX_NO_ACTIVATE);

    // sample driver inputs
    if (status == TX_SUCCESS)
    {
        while (1)
        {
            // read inputs
#if RUN_APPS_TESTBENCH
            dc_h->ts_inputs.accel_pressure = testbench_apps_input();
#else
            dc_h->ts_inputs.accel_pressure = (ULONG) apps_read();
#endif

            dc_h->ts_inputs.brake_pressure = bps_read();

            // send inputs to TS controller
            if (ts_ctrl_input_send(dc_h->ts_ctrl_h, &dc_h->ts_inputs)
                != TS_CTRL_OK)
            {
                // TODO: handle error?
                Error_Handler();
            }

            // update states all at the same time
            update_canbc_states(dc_h);

            // sleep thread to allow other threads to run
            // TODO: should the timer activation happen in the timer callback??
            //       that would give potentially more stable ticks
            tx_timer_activate(&dc_h->tick_timer);
            tx_thread_suspend(&dc_h->thread);
        }
    }
    else
    {
        dc_h->err |= DRIVER_CTRL_ERROR_INTERNAL;
        Error_Handler();
    }
}

/**
 * @brief       Completes the TS activation procedure
 *
 * @param[in]   dc_h    Driver control handle
 */
void do_activation_logic(dc_handle_t* dc_h)
{
    // TODO: visible check
    // TODO: wait for TS ON
    // TODO: activate TS ON output and LED
    // TODO: wait for TS ready
    // TODO: illuminate R2D LED
    // TODO: wait for R2D
    // TODO: optional reading from BPS
    // TODO: update broadcast states
    // TODO: ? deactivation logic
}

/**
 * @brief       Update CANBC states
 *
 * @param[in]   dc_h   Driver control handle
 */
static void update_canbc_states(dc_handle_t* dc_h)
{
    canbc_handle_t* canbc_h = dc_h->canbc_h;
    canbc_states_t* state_ptr = canbc_lock_state(canbc_h, TX_NO_WAIT);

    if (state_ptr != NULL)
    {
        state_ptr->apps_reading = dc_h->ts_inputs.accel_pressure;
        state_ptr->bps_reading = dc_h->ts_inputs.brake_pressure;

        canbc_unlock_state(canbc_h);
    }
}

/**
 * @brief       Returns true if the driver control instance has encountered an
 *              error
 *
 * @param[in]   dc_h   Driver control handle
 */
static bool no_errors(dc_handle_t* dc_h)
{
    return (dc_h->err == DRIVER_CTRL_ERROR_NONE);
}

/**
 * @brief       Create a status code based on the current error state
 *
 * @param[in]   dc_h   Driver control handle
 */
static driver_ctrl_status_t create_status(dc_handle_t* dc_h)
{
    return (no_errors(dc_h)) ? DRIVER_CTRL_OK : DRIVER_CTRL_ERROR;
}