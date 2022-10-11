/******************************************************************************
 * @file    vcu.h
 * @author  Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief   Top level VCU implementation
 *****************************************************************************/

#include "vcu.h"

#include <stdbool.h>

#include "config.h"

#include "can_rx_thread.h"
#include "can_tx_thread.h"
#include "control_thread.h"
#include "init.h"
#include "ready_to_drive.h"
#include "sensor_thread.h"
#include "watchdog_thread.h"

/*
 * macro constants
 */
#define INIT_THREAD_STACK_SIZE           1024 // TODO: needs to be profiled
#define INIT_THREAD_PREEMPTION_THRESHOLD INIT_THREAD_PRIORITY
#define INIT_THREAD_NAME                 "Initialisation Thread"

/*
 * internal function prototypes
 */
static void init_thread_entry(ULONG input);
static bool no_errors(vcu_handle_t* vcu_h);
static vcu_status_t create_status(vcu_handle_t* vcu_h);

/**
 * @brief       Initialises the VCU and all system services
 *
 * @param[in]   vcu_h           VCU instance handle
 * @param[in]   can_s_h         Sensor CAN bus handle
 * @param[in]   app_mem_pool    Pointer to RTOS application memory pool
 */
vcu_status_t vcu_init(vcu_handle_t* vcu_h,
                      CAN_HandleTypeDef* can_s_h,
                      TX_BYTE_POOL* app_mem_pool)
{
    // RTCAN service
    {
        rtcan_status_t status = rtcan_init(&vcu_h->rtcan_s,
                                           can_s_h,
                                           RTCAN_S_PRIORITY,
                                           app_mem_pool);

        if (status == RTCAN_OK)
        {
            status = rtcan_start(&vcu_h->rtcan_s);
        }

        if (status != RTCAN_OK)
        {
            vcu_h->err |= VCU_ERROR_INIT;
        }
    }

    // CAN broadcast service
    if (no_errors(vcu_h))
    {
        canbc_status_t status = canbc_init(&vcu_h->canbc,
                                           &vcu_h->rtcan_s,
                                           CANBC_PRIORITY,
                                           app_mem_pool);

        if (status == CANBC_OK)
        {
            status = canbc_start(&vcu_h->canbc);
        }

        if (status != CANBC_OK)
        {
            vcu_h->err |= VCU_ERROR_INIT;
        }
    }

    // initialisation thread
    if (no_errors(vcu_h))
    {
        void* stack_ptr = NULL;

        UINT status = tx_byte_allocate(app_mem_pool,
                                       &stack_ptr,
                                       INIT_THREAD_STACK_SIZE,
                                       TX_NO_WAIT);

        if (status == TX_SUCCESS)
        {
            status = tx_thread_create(&vcu_h->init_thread,
                                      INIT_THREAD_NAME,
                                      init_thread_entry,
                                      0,
                                      stack_ptr,
                                      INIT_THREAD_STACK_SIZE,
                                      INIT_THREAD_PRIORITY,
                                      INIT_THREAD_PREEMPTION_THRESHOLD,
                                      TX_NO_TIME_SLICE,
                                      TX_AUTO_START); // !!! auto started
        }

        if (status != TX_SUCCESS)
        {
            vcu_h->err |= VCU_ERROR_INIT;
        }
    }

    return create_status(vcu_h);
}

/**
 * @brief       Handles CAN transmit mailbox callbacks
 *
 * @param[in]   vcu_h   VCU handle
 * @param[in]   can_h   CAN handle from callback
 */
vcu_status_t vcu_handle_can_tx_mailbox_callback(vcu_handle_t* vcu_h,
                                                CAN_HandleTypeDef* can_h)
{
    rtcan_status_t status
        = rtcan_handle_tx_mailbox_callback(&vcu_h->rtcan_s, can_h);

    if (status != RTCAN_OK)
    {
        vcu_h->err |= VCU_ERROR_PERIPHERAL;
    }

    return create_status(vcu_h);
}

/**
 * @brief       Initialisation thread entry function
 *
 * @details     This begins running as soon as the RTOS kernel is entered.
 *              The initialisation thread then has the following
 *              responsibilities:
 *
 *                  1. Complete the pre ready-to-drive initialisation.
 *                  2. Wait for the ready-to-drive state to be entered.
 *                  3. Complete the post ready-to-drive initialisation.
 *                  4. Terminate itself and launch all other threads.
 *
 *              Any service threads such as RTCAN and CANBC are started along
 *              with this thread. Application threads should not run until this
 *              thread is terminated.
 *
 * @param[in]   input   VCU handle
 */
static void init_thread_entry(ULONG input)
{
    vcu_handle_t* vcu_h = (vcu_handle_t*) input;

    // RTD procedure
    init_pre_rtd();
    rtd_wait();
    init_post_rtd();

    // start all the application threads
    // TODO: this will change once the other threads are wrapped in context
    //       structs
    UINT(*thread_start_funcs[])
    () = {can_tx_thread_start, control_thread_start, sensor_thread_start};

    const UINT num_to_start
        = sizeof(thread_start_funcs) / sizeof(thread_start_funcs[0]);

    for (UINT i = 0; i < num_to_start; i++)
    {
        thread_start_funcs[i]();
    }

    // terminate this thread
    (void) tx_thread_terminate(&vcu_h->init_thread);
}

/**
 * @brief       Returns true if the VCU instance has encountered an error
 *
 * @param[in]   vcu_h   VCU handle
 */
static bool no_errors(vcu_handle_t* vcu_h)
{
    return (vcu_h->err == VCU_ERROR_NONE);
}

/**
 * @brief       Create a status code based on the current error state
 *
 * @param[in]   vcu_h   VCU handle
 */
static vcu_status_t create_status(vcu_handle_t* vcu_h)
{
    return (no_errors(vcu_h)) ? VCU_OK : VCU_ERROR;
}