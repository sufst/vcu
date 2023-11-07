/*****************************************************************************
 * @file   driver_control.c
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief  Driver control
 ****************************************************************************/

#include "driver_control.h"

#include <stdbool.h>

#include "config.h"

#include "apps.h"
#include "bps.h"

#include "Test/testbench.h"

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
static bool no_errors(driver_ctrl_handle_t* driver_ctrl_h);
static driver_ctrl_status_t create_status(driver_ctrl_handle_t* driver_ctrl_h);

/**
 * @brief       Initialises driver controls
 *
 * @param[in]   driver_ctrl_h   Driver control handle
 * @param[in]   ts_ctrl_h       TS controller handle
 * @param[in]   canbc_h         CANBC service handle
 * @param[in]   stack_pool_ptr  Memory pool to allocate stack memory from
 */
driver_ctrl_status_t driver_ctrl_init(driver_ctrl_handle_t* driver_ctrl_h,
                                      ts_ctrl_handle_t* ts_ctrl_h,
                                      canbc_handle_t* canbc_h,
                                      TX_BYTE_POOL* stack_pool_ptr)
{
    driver_ctrl_h->ts_ctrl_h = ts_ctrl_h;

    driver_ctrl_h->ts_inputs.accel_pressure = 0;
    driver_ctrl_h->ts_inputs.brake_pressure = 0;

    // thread
    {
        void* stack_ptr = NULL;

        UINT status = tx_byte_allocate(stack_pool_ptr,
                                       &stack_ptr,
                                       DRIVER_CTRL_THREAD_STACK_SIZE,
                                       TX_NO_WAIT);

        if (status == TX_SUCCESS)
        {
            status = tx_thread_create(&driver_ctrl_h->thread,
                                      DRIVER_CTRL_THREAD_NAME,
                                      driver_ctrl_thread_entry,
                                      (ULONG) driver_ctrl_h,
                                      stack_ptr,
                                      DRIVER_CTRL_THREAD_STACK_SIZE,
                                      DRIVER_CTRL_THREAD_PRIORITY,
                                      DRIVER_CTRL_THREAD_PRIORITY,
                                      TX_NO_TIME_SLICE,
                                      TX_DONT_START);
        }

        if (status != TX_SUCCESS)
        {
            driver_ctrl_h->err |= DRIVER_CTRL_ERROR_INIT;
        }
    }

    // can broadcast service
    if (no_errors(driver_ctrl_h))
    {
        UINT status = tx_mutex_create(&driver_ctrl_h->state_mutex,
                                      DRIVER_CTRL_STATE_MUTEX_NAME,
                                      TX_INHERIT);

        if (status != TX_SUCCESS)
        {
            driver_ctrl_h->err |= DRIVER_CTRL_ERROR_INIT;
        }
    }

    if (no_errors(driver_ctrl_h))
    {
        canbc_segment_t* segment_ptr;
        canbc_status_t status
            = canbc_create_segment(canbc_h,
                                   &segment_ptr,
                                   CANBC_DRIVER_INPUTS_ID,
                                   &driver_ctrl_h->state_mutex);

        if (status == CANBC_OK)
        {
            // TODO: channel_ptr unused, change CANBC to ignore parameter if
            //       NULL
            canbc_channel_t* channel_ptr;
            status = canbc_create_channel(
                canbc_h,
                &channel_ptr,
                segment_ptr,
                (uint8_t*) &driver_ctrl_h->ts_inputs.accel_pressure,
                sizeof(driver_ctrl_h->ts_inputs.accel_pressure));
        }

        if (status == CANBC_OK)
        {
            canbc_channel_t* channel_ptr;
            status = canbc_create_channel(
                canbc_h,
                &channel_ptr,
                segment_ptr,
                (uint8_t*) &driver_ctrl_h->ts_inputs.brake_pressure,
                sizeof(driver_ctrl_h->ts_inputs.brake_pressure));
        }

        if (status != CANBC_OK)
        {
            driver_ctrl_h->err |= DRIVER_CTRL_ERROR_INIT;
        }
    }

    return create_status(driver_ctrl_h);
}

/**
 * @brief       Start reading driver controls
 *
 * @param[in]   driver_ctrl_h   Driver control handle
 */
driver_ctrl_status_t driver_ctrl_start(driver_ctrl_handle_t* driver_ctrl_h)
{
    UINT status = tx_thread_resume(&driver_ctrl_h->thread);

    if (status != TX_SUCCESS)
    {
        driver_ctrl_h->err |= DRIVER_CTRL_ERROR_INTERNAL;
    }

    return create_status(driver_ctrl_h);
}

/**
 * @brief       Tick timer callback
 *
 * @param[in]   input   Driver control handle
 */
static void driver_ctrl_tick_callback(ULONG input)
{
    driver_ctrl_handle_t* driver_ctrl_h = (driver_ctrl_handle_t*) input;

    UINT status = tx_thread_resume(&driver_ctrl_h->thread);

    if (status != TX_SUCCESS)
    {
        driver_ctrl_h->err |= DRIVER_CTRL_ERROR_INTERNAL;
    }
}

/**
 * @brief       Driver control thread entry function
 *
 * @param[in]   input   Driver control handle
 */
static void driver_ctrl_thread_entry(ULONG input)
{
    driver_ctrl_handle_t* driver_ctrl_h = (driver_ctrl_handle_t*) input;

    // create tick timer
    const ULONG ticks = TX_TIMER_TICKS_PER_SECOND / DRIVER_CTRL_TICK_RATE;

    UINT status = tx_timer_create(&driver_ctrl_h->tick_timer,
                                  DRIVER_CTRL_TIMER_NAME,
                                  driver_ctrl_tick_callback,
                                  (ULONG) driver_ctrl_h,
                                  ticks,
                                  ticks,
                                  TX_NO_ACTIVATE);

    // sample driver inputs
    if (status == TX_SUCCESS)
    {
        while (1)
        {
            // read inputs
            // TODO: handle error return
            tx_mutex_get(&driver_ctrl_h->state_mutex, TX_WAIT_FOREVER);

#if RUN_APPS_TESTBENCH
            driver_ctrl_h->ts_inputs.accel_pressure = testbench_apps_input();
#else
            driver_ctrl_h->ts_inputs.accel_pressure = (ULONG) apps_read();
#endif

            driver_ctrl_h->ts_inputs.brake_pressure = bps_read();

            // send inputs to TS controller
            if (ts_ctrl_input_send(driver_ctrl_h->ts_ctrl_h,
                                   &driver_ctrl_h->ts_inputs)
                != TS_CTRL_OK)
            {
                // TODO: handle error?
            }

            tx_mutex_put(&driver_ctrl_h->state_mutex);

            // sleep thread to allow other threads to run
            // TODO: should the timer activation happen in the timer callback??
            //       that would give potentially more stable ticks
            tx_timer_activate(&driver_ctrl_h->tick_timer);
            tx_thread_suspend(&driver_ctrl_h->thread);
        }
    }
    else
    {
        driver_ctrl_h->err |= DRIVER_CTRL_ERROR_INTERNAL;
        // TODO: handle fault
    }
}

/**
 * @brief       Returns true if the driver control instance has encountered an
 *              error
 *
 * @param[in]   driver_ctrl_h   Driver control handle
 */
static bool no_errors(driver_ctrl_handle_t* driver_ctrl_h)
{
    return (driver_ctrl_h->err == DRIVER_CTRL_ERROR_NONE);
}

/**
 * @brief       Create a status code based on the current error state
 *
 * @param[in]   driver_ctrl_h   Driver control handle
 */
static driver_ctrl_status_t create_status(driver_ctrl_handle_t* driver_ctrl_h)
{
    return (no_errors(driver_ctrl_h)) ? DRIVER_CTRL_OK : DRIVER_CTRL_ERROR;
}