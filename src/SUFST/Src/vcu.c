/******************************************************************************
 * @file    vcu.h
 * @author  Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @author  Toby Godfrey (@_tg03, tmag1g21@soton.ac.uk)
 * @author  George Peppard (@inventor02, gjp1g21@soton.ac.uk)
 * @brief   Top level VCU implementation
 *****************************************************************************/

#include "vcu.h"

#include <stdbool.h>

#include "apps.h"
#include "bps.h"
#include "config.h"
#include "dash.h"

/**
 * @brief       Initialises the VCU and all system services
 *
 * @param[in]   vcu_ptr         VCU instance
 * @param[in]   can_c_h         Critical systems CAN bus handle
 * @param[in]   can_s_h         Sensor CAN bus handle
 * @param[in]   app_mem_pool    Pointer to RTOS application memory pool
 * @param[in]   config_ptr      Pointer to VCU configuration instance
 */
status_t vcu_init(vcu_context_t *vcu_ptr,
                  CAN_HandleTypeDef *can_c_h,
                  CAN_HandleTypeDef *can_s_h,
                  TX_BYTE_POOL *app_mem_pool,
                  const config_t *config_ptr)
{
    vcu_ptr->config_ptr = config_ptr;

    status_t status = STATUS_OK;

    // logging services (first so we can log errors)
    if (status == STATUS_OK)
    {
        status = log_init(&vcu_ptr->log, app_mem_pool, &vcu_ptr->config_ptr->log);
    }

    // RTCAN services
    rtcan_handle_t *rtcan_handles[] = {&vcu_ptr->rtcan_s, &vcu_ptr->rtcan_c};
    CAN_HandleTypeDef *can_handles[] = {can_s_h, can_c_h};
    ULONG rtcan_priorities[] = {vcu_ptr->config_ptr->rtos.rtcan_s_priority,
                                vcu_ptr->config_ptr->rtos.rtcan_c_priority};

    for (uint32_t i = 0; i < 2; i++)
    {
        if (status == STATUS_OK)
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
                // TODO: error
                status = STATUS_ERROR;
                // vcu_h->err |= VCU_ERROR_INIT;
            }
            else
            {
                LOG_INFO("RTCAN service %d started\n", i);
            }
        }
    }

    // CAN broadcast service
    if (status == STATUS_OK)
    {
        status = canbc_init(&vcu_ptr->canbc,
                            &vcu_ptr->rtcan_s,
                            app_mem_pool,
                            &vcu_ptr->config_ptr->canbc);
    }

    // dash
    if (status == STATUS_OK)
    {
        status = dash_init(&vcu_ptr->dash,
                           app_mem_pool,
                           &vcu_ptr->config_ptr->dash);
    }

    // tick
    if (status == STATUS_OK)
    {
        status = tick_init(&vcu_ptr->tick,
                           &vcu_ptr->canbc,
                           app_mem_pool,
                           &vcu_ptr->config_ptr->tick,
                           &vcu_ptr->config_ptr->apps,
                           &vcu_ptr->config_ptr->bps);
    }

    // pm100
    if (status == STATUS_OK)
    {
        status = pm100_init(&vcu_ptr->pm100,
                            app_mem_pool,
                            &vcu_ptr->rtcan_c,
                            &vcu_ptr->rtcan_s,
                            &vcu_ptr->config_ptr->pm100);
    }

    // control
    if (status == STATUS_OK)
    {
        status = ctrl_init(&vcu_ptr->ctrl,
                           &vcu_ptr->dash,
                           &vcu_ptr->pm100,
                           &vcu_ptr->tick,
                           &vcu_ptr->remote_ctrl,
                           &vcu_ptr->canbc,
                           app_mem_pool,
                           &vcu_ptr->config_ptr->ctrl,
                           &vcu_ptr->config_ptr->rtds,
                           &vcu_ptr->config_ptr->torque_map);
    }

    // remote control
    if (status == STATUS_OK)
    {
        status = remote_ctrl_init(
            &vcu_ptr->remote_ctrl,
            &vcu_ptr->canbc,
            app_mem_pool,
            &vcu_ptr->rtcan_s,
            &vcu_ptr->config_ptr->remote_ctrl);
        LOG_INFO("Remote control service initialised\n");
    }

    if (status != STATUS_OK)
    {
        LOG_ERROR("Failed to initialise remote control service\n");
    }

    // heartbeat
    if (status == STATUS_OK)
    {
        status = heartbeat_init(&vcu_ptr->heartbeat,
                                app_mem_pool,
                                &vcu_ptr->config_ptr->heartbeat);
    }

    if (status != STATUS_OK)
        LOG_ERROR("Some services failed to initialise\n");

    LOG_INFO("All VCU services initialised\n");
    return status;
}

/**
 * @brief       Handles CAN transmit mailbox callbacks
 *
 * @param[in]   vcu_ptr VCU instance
 * @param[in]   can_h   CAN handle from callback
 */
status_t vcu_handle_can_tx_mailbox_callback(vcu_context_t *vcu_ptr,
                                            CAN_HandleTypeDef *can_h)
{
    // TODO: how to handle CAN C vs CAN S
    rtcan_status_t status = rtcan_handle_tx_mailbox_callback(&vcu_ptr->rtcan_c, can_h);

    if (status != RTCAN_OK)
    {
        // TODO: error
        // vcu_h->err |= VCU_ERROR_PERIPHERAL;
        Error_Handler();
    }

    status = rtcan_handle_tx_mailbox_callback(&vcu_ptr->rtcan_s, can_h);

    if (status != RTCAN_OK)
    {
        // TODO: error
        // vcu_h->err |= VCU_ERROR_PERIPHERAL;
        Error_Handler();
    }

    return STATUS_OK; //? Error_Handler() never returns so this can only return
                      // STATUS_OK
}

/**
 * @brief       Handles CAN receive interrupt
 *
 * @param[in]   vcu_ptr     VCU instance
 * @param[in]   can_h       CAN handle
 * @param[in]   rx_fifo     Rx FIFO number
 */
status_t vcu_handle_can_rx_it(vcu_context_t *vcu_ptr,
                              CAN_HandleTypeDef *can_h,
                              uint32_t rx_fifo)
{
    rtcan_status_t status;

    if (vcu_ptr->rtcan_c.hcan == can_h)
    {
        status = rtcan_handle_rx_it(&vcu_ptr->rtcan_c, can_h, rx_fifo);
    }
    else if (vcu_ptr->rtcan_s.hcan == can_h)
    {
        status = rtcan_handle_rx_it(&vcu_ptr->rtcan_s, can_h, rx_fifo);
    }

    if (status != RTCAN_OK)
    {
        // TODO: error?
        // Error_Handler();
    }

    return STATUS_OK; // TODO: errors
}

/**
 * @brief       Handles CAN errors
 *
 * @param[in]   vcu_ptr     VCU instance
 * @param[in]   can_h       CAN handle from callback
 */
status_t vcu_handle_can_err(vcu_context_t *vcu_ptr, CAN_HandleTypeDef *can_h)
{
    rtcan_status_t status;

    if (vcu_ptr->rtcan_c.hcan == can_h)
    {
        status = rtcan_handle_hal_error(&vcu_ptr->rtcan_c, can_h);
    }
    else if (vcu_ptr->rtcan_s.hcan == can_h)
    {
        status = rtcan_handle_hal_error(&vcu_ptr->rtcan_s, can_h);
    }

    if (status != RTCAN_OK)
    {
        // TODO: error
        // vcu_h->err |= VCU_ERROR_PERIPHERAL;
        Error_Handler();
    }

    return STATUS_OK;
}
