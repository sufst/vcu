/***************************************************************************
 * @file   ts_control.c
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @brief  Tractive System (TS) controller
 ***************************************************************************/

#include "ts_control.h"

#include <stdbool.h>

#include "config.h"

#include "can_c.h"
#include "can_s.h"
#include "driver_profiles.h"

/*
 * macro constants
 */
#define TS_CTRL_THREAD_STACK_SIZE 1024

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
 * @param[in]   rtcan_h         RTCAN handle for inverter communications
 * @param[in]   stack_pool_ptr  Memory pool to allocate stack memory from
 */
ts_ctrl_status_t ts_ctrl_init(ts_ctrl_handle_t* ts_ctrl_h,
                              rtcan_handle_t* rtcan_h,
                              TX_BYTE_POOL* stack_pool_ptr)
{
    ts_ctrl_h->rtcan_h = rtcan_h;

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
                                      "Tractive System Control Thread",
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
                                      "Tractive System Control Input Queue",
                                      sizeof(ts_ctrl_input_t) / sizeof(ULONG),
                                      ts_ctrl_h->input_queue_mem,
                                      sizeof(ts_ctrl_h->input_queue_mem));

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
    if (no_errors(ts_ctrl_h))
    {
        UINT status = tx_thread_resume(&ts_ctrl_h->thread);

        if (status != TX_SUCCESS)
        {
            ts_ctrl_h->err |= TS_CTRL_ERROR_INTERNAL;
        }
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
    // -> if the inverter is in lockout, new inputs are ignored by this
    if (no_errors(ts_ctrl_h))
    {
        status = TX_SUCCESS;

        if (enqueued != 0)
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

    // initialise inverter driver
    if (no_errors(ts_ctrl_h))
    {
        status_t status = pm100_init(&ts_ctrl_h->pm100, ts_ctrl_h->rtcan_h);

        if (status != STATUS_OK)
        {
            ts_ctrl_h->err |= TS_CTRL_ERROR_INIT;
        }
    }

    // look-up the driver profile
    // (note this is fixed at compile time at the moment)
    const driver_profile_t* driver_profile_ptr;

    if (driver_profile_lookup(&driver_profile_ptr, SELECTED_DRIVER_PROFILE)
        != DRIVER_PROFILE_FOUND)
    {
        // TODO: create fault handling system
        Error_Handler();
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

            torque_request /= 10;

            // handle any incoming CAN messages before creating a torque request
            pm100_process_broadcast_msgs(&ts_ctrl_h->pm100);

            // send the torque request
            status_t status
                = pm100_request_torque(&ts_ctrl_h->pm100, torque_request);

            if (status != STATUS_OK)
            {
                // TODO: proper error handler!
                Error_Handler();
            }
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
