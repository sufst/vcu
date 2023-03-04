/******************************************************************************
 * @file    vcu.h
 * @author  Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief   Top level VCU implementation
 *****************************************************************************/

#include "vcu.h"

#include <stdbool.h>

#include "config.h"

#include "apps.h"
#include "bps.h"

#include "ready_to_drive.h"

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
 * @param[in]   can_c_h         Critical systems CAN bus handle
 * @param[in]   can_s_h         Sensor CAN bus handle
 * @param[in]   app_mem_pool    Pointer to RTOS application memory pool
 */
vcu_status_t vcu_init(vcu_handle_t* vcu_h,
                      CAN_HandleTypeDef* can_c_h,
                      CAN_HandleTypeDef* can_s_h,
                      TX_BYTE_POOL* app_mem_pool)
{
    vcu_h->err = VCU_ERROR_NONE;

    // RTCAN services
    rtcan_handle_t* rtcan_handles[] = {&vcu_h->rtcan_s, &vcu_h->rtcan_c};
    CAN_HandleTypeDef* can_handles[] = {can_s_h, can_c_h};
    ULONG rtcan_priorities[] = {RTCAN_S_PRIORITY, RTCAN_C_PRIORITY};

    for (uint32_t i = 0; i < 2; i++)
    {
        if (no_errors(vcu_h))
        {
            rtcan_status_t status = rtcan_init(rtcan_handles[i],
                                               can_handles[i],
                                               rtcan_priorities[i],
                                               app_mem_pool);

            if (status == RTCAN_OK)
            {
                status = rtcan_start(rtcan_handles[i]);
            }

            if (status != RTCAN_OK)
            {
                vcu_h->err |= VCU_ERROR_INIT;
            }
        }
    }

    // CAN broadcast service
    if (no_errors(vcu_h))
    {
        canbc_status_t status = canbc_init(&vcu_h->canbc,
                                           &vcu_h->rtcan_s,
                                           CANBC_PRIORITY,
                                           CANBC_BROADCAST_PERIOD,
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
                                      (ULONG) vcu_h,
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

    // tractive system controller
    if (no_errors(vcu_h))
    {
        ts_ctrl_status_t status
            = ts_ctrl_init(&vcu_h->ts_ctrl, &vcu_h->rtcan_c, app_mem_pool);

        if (status != TS_CTRL_OK)
        {
            vcu_h->err |= VCU_ERROR_INIT;
        }
    }

    // driver control input service
    if (no_errors(vcu_h))
    {
        driver_ctrl_status_t status = driver_ctrl_init(&vcu_h->driver_ctrl,
                                                       &vcu_h->ts_ctrl,
                                                       &vcu_h->canbc,
                                                       app_mem_pool);

        if (status != DRIVER_CTRL_OK)
        {
            vcu_h->err |= VCU_ERROR_INIT;
        }
    }

    // ready to drive
    if (no_errors(vcu_h))
    {
        const uint32_t speaker_ticks
            = (READY_TO_DRIVE_SPEAKER_TIME * TX_TIMER_TICKS_PER_SECOND) / 1000;

        rtd_init(&vcu_h->rtd,
                 SPKR_GPIO_Port,
                 SPKR_Pin,
                 speaker_ticks,
                 READY_TO_DRIVE_CHECK_BPS);
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
    // TODO: how to handle CAN C vs CAN S
    rtcan_status_t status
        = rtcan_handle_tx_mailbox_callback(&vcu_h->rtcan_c, can_h);

    if (status != RTCAN_OK)
    {
        vcu_h->err |= VCU_ERROR_PERIPHERAL;
    }

    status = rtcan_handle_tx_mailbox_callback(&vcu_h->rtcan_s, can_h);

    if (status != RTCAN_OK)
    {
        vcu_h->err |= VCU_ERROR_PERIPHERAL;
    }

    return create_status(vcu_h);
}

/**
 * @brief       Handles CAN receive interrupt
 *
 * @param[in]   vcu_h       VCU handle
 * @param[in]   can_h       CAN handle
 * @param[in]   rx_fifo     Rx FIFO number
 */
vcu_status_t vcu_handle_can_rx_it(vcu_handle_t* vcu_h,
                                  CAN_HandleTypeDef* can_h,
                                  uint32_t rx_fifo)
{
    rtcan_status_t status;

    if (vcu_h->rtcan_c.hcan == can_h)
    {
        status = rtcan_handle_rx_it(&vcu_h->rtcan_c, can_h, rx_fifo);
    }
    else if (vcu_h->rtcan_s.hcan == can_h)
    {
        status = rtcan_handle_rx_it(&vcu_h->rtcan_s, can_h, rx_fifo);
    }

    if (status != RTCAN_OK)
    {
        vcu_h->err |= VCU_ERROR_PERIPHERAL;
    }

    return create_status(vcu_h);
}

/**
 * @brief       Handles CAN errors
 *
 * @param[in]   vcu_h
 * @param[in]   can_h
 */
vcu_status_t vcu_handle_can_err(vcu_handle_t* vcu_h, CAN_HandleTypeDef* can_h)
{
    rtcan_status_t status;

    if (vcu_h->rtcan_c.hcan == can_h)
    {
        status = rtcan_handle_hal_error(&vcu_h->rtcan_c, can_h);
    }
    else if (vcu_h->rtcan_s.hcan == can_h)
    {
        status = rtcan_handle_hal_error(&vcu_h->rtcan_s, can_h);
    }

    if (status != RTCAN_OK)
    {
        vcu_h->err |= VCU_ERROR_PERIPHERAL;
    }

    return create_status(vcu_h);
}

vcu_status_t vcu_handle_exti_callback(vcu_handle_t* vcu_h, uint16_t pin)
{
    if ((pin == RTD_IN_Pin)
        || (pin == USER_BUTTON_Pin && READY_TO_DRIVE_BUTTON_ENABLE))
    {
        rtd_handle_int(&vcu_h->rtd);
    }

    return VCU_OK;
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

    bps_init();
    apps_init();

    rtd_wait(&vcu_h->rtd);

    // TODO: handle errors
    (void) ts_ctrl_start(&vcu_h->ts_ctrl);
    (void) driver_ctrl_start(&vcu_h->driver_ctrl);

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