/***************************************************************************
 * @file   ts_control.c
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @brief  Tractive System (TS) controller
 ***************************************************************************/

#include "ts_control.h"

#include <stdbool.h>

#include "config.h"

#include "fault.h"

#include "driver_profiles.h"

/*
 * macro constants
 */
#define TS_CTRL_THREAD_STACK_SIZE 1024
#define TS_CTRL_THREAD_NAME       "Tractive System Control Thread"
#define TS_CTRL_INPUT_QUEUE_NAME  "Tractive System Control Input Queue"

/*
 * internal functions
 */
static void ts_ctrl_thread_entry(ULONG input);
static bool no_errors(ts_ctrl_handle_t* ts_ctrl_h);
static ts_ctrl_status_t create_status(ts_ctrl_handle_t* ts_ctrl_h);

/**
 * @brief       Initialises the TS controller
 *
 * @param[in]   ts_ctrl_h       TS controller handle
 * @param[in]   stack_pool_ptr  Memory pool to allocate stack memory from
 */
ts_ctrl_status_t ts_ctrl_init(ts_ctrl_handle_t* ts_ctrl_h,
                              TX_BYTE_POOL* stack_pool_ptr)
{
    // create thread
    {
        void* stack_ptr = NULL;

        UINT status = tx_byte_allocate(stack_pool_ptr,
                                       &stack_ptr,
                                       TS_CTRL_THREAD_STACK_SIZE,
                                       TX_NO_WAIT);

        if (status == TX_SUCCESS)
        {
            status = tx_thread_create(&ts_ctrl_h->thread,
                                      TS_CTRL_THREAD_NAME,
                                      ts_ctrl_thread_entry,
                                      (ULONG) ts_ctrl_h,
                                      stack_ptr,
                                      TS_CTRL_THREAD_STACK_SIZE,
                                      TS_CTRL_THREAD_PRIORITY,
                                      TS_CTRL_THREAD_PRIORITY,
                                      TX_NO_TIME_SLICE,
                                      TX_DONT_START);
        }

        if (status != TX_SUCCESS)
        {
            ts_ctrl_h->err |= TS_CTRL_ERROR_INIT;
        }
    }

    // create input queue
    if (no_errors(ts_ctrl_h))
    {
        UINT status = tx_queue_create(&ts_ctrl_h->input_queue,
                                      TS_CTRL_INPUT_QUEUE_NAME,
                                      TS_CTRL_INPUT_QUEUE_ITEM_SIZE,
                                      ts_ctrl_h->input_queue_mem,
                                      TS_CTRL_INPUT_QUEUE_SIZE);

        if (status != TX_SUCCESS)
        {
            ts_ctrl_h->err |= TS_CTRL_ERROR_INIT;
        }
    }

    return create_status(ts_ctrl_h);
}

/**
 * @brief   Starts the TS controller
 */
ts_ctrl_status_t ts_ctrl_start(ts_ctrl_handle_t* ts_ctrl_h)
{
    UINT status = tx_thread_resume(&ts_ctrl_h->thread);

    if (status != TX_SUCCESS)
    {
        ts_ctrl_h->err |= TS_CTRL_ERROR_INTERNAL;
    }

    return create_status(ts_ctrl_h);
}

/**
 * @brief       Sends an input to the TS controller
 *
 * @details     If the controller has not processed the previously sent input,
 *              it will be replaced with this new input
 *
 * @param[in]   ts_ctrl_h   TS controller handle
 * @param[in]   input_ptr   Pointer to input
 */
ts_ctrl_status_t ts_ctrl_input_send(ts_ctrl_handle_t* ts_ctrl_h,
                                    ts_ctrl_input_t* input_ptr)
{
    // check if there is anything in the queue already (bad!)
    ULONG enqueued;

    UINT status = tx_queue_info_get(&ts_ctrl_h->input_queue,
                                    NULL,
                                    &enqueued,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL);

    if (status != TX_SUCCESS)
    {
        ts_ctrl_h->err |= TS_CTRL_ERROR_INTERNAL;
    }

    // post to the queue, flushing it if not empty
    // -> the TS controller should have the most up to date inputs
    if (no_errors(ts_ctrl_h))
    {
        status = TX_SUCCESS;

        if (enqueued
            != 0) // TODO: this really shouldn't happen - should log this?
        {
            status = tx_queue_flush(&ts_ctrl_h->input_queue);
        }

        if (status == TX_SUCCESS)
        {
            status
                = tx_queue_send(&ts_ctrl_h->input_queue, input_ptr, TX_NO_WAIT);
        }

        if (status != TX_SUCCESS)
        {
            ts_ctrl_h->err |= TS_CTRL_ERROR_INTERNAL;
        }
    }

    return create_status(ts_ctrl_h);
}

/**
 * @brief       TS controller thread entry function
 *
 * @param[in]   input   TS controller handle
 */
static void ts_ctrl_thread_entry(ULONG input)
{
    ts_ctrl_handle_t* ts_ctrl_h = (ts_ctrl_handle_t*) input;

    // look-up the driver profile
    // (note this is fixed at compile time at the moment)
    const driver_profile_t* driver_profile_ptr;

    if (driver_profile_lookup(&driver_profile_ptr, SELECTED_DRIVER_PROFILE)
        != DRIVER_PROFILE_FOUND)
    {
        critical_fault(CRITICAL_FAULT_DRIVER_PROFILE_NOT_FOUND);
    }

    // loop forever
    while (1)
    {
        // wait for a message to enter the control input queue
        // -> thread suspended until message received
        ts_ctrl_input_t inputs;

        if (tx_queue_receive(&ts_ctrl_h->input_queue,
                             (void*) &inputs,
                             TX_WAIT_FOREVER)
            == TX_SUCCESS)
        {
            // apply the thottle curve
            UINT torque_request
                = apply_torque_map(driver_profile_ptr, inputs.accel_pressure);

            // TODO: send the torque request!
            (void) torque_request;
        }
    }
}

/**
 * @brief       Returns true if the TS controller instance has encountered an
 *              error
 *
 * @param[in]   ts_ctrl_h   TS controller handle
 */
static bool no_errors(ts_ctrl_handle_t* ts_ctrl_h)
{
    return (ts_ctrl_h->err == TS_CTRL_ERROR_NONE);
}

/**
 * @brief       Create a status code based on the current error state
 *
 * @param[in]   ts_ctrl_h   TS controller handle
 */
static ts_ctrl_status_t create_status(ts_ctrl_handle_t* ts_ctrl_h)
{
    return (no_errors(ts_ctrl_h)) ? TS_CTRL_OK : TS_CTRL_ERROR;
}