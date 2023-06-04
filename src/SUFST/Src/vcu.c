/******************************************************************************
 * @file    vcu.h
 * @author  Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief   Top level VCU implementation
 *****************************************************************************/

#include "vcu.h"

#include <stdbool.h>

#include "apps.h"
#include "bps.h"
#include "config.h"
#include "dash.h"

/*
 * internal function prototypes
 */
static bool no_errors(vcu_handle_t* vcu_h);
static vcu_status_t create_status(vcu_handle_t* vcu_h);

/**
 * @brief       Initialises the VCU and all system services
 *
 * @param[in]   vcu_h           VCU instance handle
 * @param[in]   can_c_h         Critical systems CAN bus handle
 * @param[in]   can_s_h         Sensor CAN bus handle
 * @param[in]   app_mem_pool    Pointer to RTOS application memory pool
 * @param[in]   config_ptr      Pointer to VCU configuration instance
 */
vcu_status_t vcu_init(vcu_handle_t* vcu_h,
                      CAN_HandleTypeDef* can_c_h,
                      CAN_HandleTypeDef* can_s_h,
                      TX_BYTE_POOL* app_mem_pool,
                      const config_t* config_ptr)
{
    vcu_h->err = VCU_ERROR_NONE;
    vcu_h->config_ptr = config_ptr;

    status_t status = STATUS_OK;

    // RTCAN services
    rtcan_handle_t* rtcan_handles[] = {&vcu_h->rtcan_s, &vcu_h->rtcan_c};
    CAN_HandleTypeDef* can_handles[] = {can_s_h, can_c_h};
    ULONG rtcan_priorities[] = {RTCAN_S_PRIORITY, RTCAN_C_PRIORITY};

    for (uint32_t i = 0; i < 2; i++)
    {
        if (no_errors(vcu_h))
        {
            rtcan_status_t rtcan_status = rtcan_init(rtcan_handles[i],
                                                     can_handles[i],
                                                     rtcan_priorities[i],
                                                     app_mem_pool);

            if (rtcan_status == RTCAN_OK)
            {
                rtcan_status = rtcan_start(rtcan_handles[i]);
            }

            if (rtcan_status != RTCAN_OK)
            {
                vcu_h->err |= VCU_ERROR_INIT;
            }
        }
    }

    // CAN broadcast service
    if (no_errors(vcu_h))
    {
        canbc_init(&vcu_h->canbc,
                   &vcu_h->rtcan_c,
                   app_mem_pool,
                   &vcu_h->config_ptr->canbc);
    }

    // dash
    if (status == STATUS_OK)
    {
        status
            = dash_init(&vcu_h->dash, app_mem_pool, &vcu_h->config_ptr->dash);
    }

    // control
    if (status == STATUS_OK)
    {
        status = ctrl_init(&vcu_h->ctrl,
                           &vcu_h->dash,
                           &vcu_h->canbc,
                           app_mem_pool,
                           &vcu_h->config_ptr->ctrl,
                           &vcu_h->config_ptr->rtds);
    }

    UNUSED(status);

    return VCU_OK; // TODO: return status_T
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
        Error_Handler();
    }

    status = rtcan_handle_tx_mailbox_callback(&vcu_h->rtcan_s, can_h);

    if (status != RTCAN_OK)
    {
        vcu_h->err |= VCU_ERROR_PERIPHERAL;
        Error_Handler();
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
        // Error_Handler();
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
        Error_Handler();
    }

    return create_status(vcu_h);
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