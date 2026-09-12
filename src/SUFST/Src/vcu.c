/******************************************************************************
 * @file    vcu.h
 * @author  Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @author  Toby Godfrey (@_tg03, tmag1g21@soton.ac.uk)
 * @author  George Peppard (@inventor02, gjp1g21@soton.ac.uk)
 * @brief   Top level VCU implementation
 *****************************************************************************/

#include "vcu.h"

#include <stdbool.h>

/**
 * @brief       Initialises the VCU and all system services
 *
 * @param[in]   vcu_ptr         VCU instance
 * @param[in]   can_t_h         Critical systems CAN bus handle
 * @param[in]   can_s_h         Sensor CAN bus handle
 * @param[in]   app_mem_pool    Pointer to RTOS application memory pool
 * @param[in]   config_ptr      Pointer to VCU configuration instance
 */
status_t vcu_init(vcu_context_t *vcu_ptr,
                  CAN_HandleTypeDef *can_t_h,
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

    // heartbeat
    if (status == STATUS_OK)
    {
        status = heartbeat_init(&vcu_ptr->heartbeat, app_mem_pool,
                                &vcu_ptr->config_ptr->heartbeat);
    }

    // enter USB mass storage mode if requested
    if (status == STATUS_OK && usb_mass_storage_mode_button_held())
    {
        return usb_msc_init(&vcu_ptr->usb_msc, app_mem_pool,
                            &vcu_ptr->config_ptr->usb_msc);
    }

    // RTCAN services
    static const CAN_FilterTypeDef can_s_filters[] = {
        {
            .FilterIdHigh = 0x0000,
            .FilterIdLow = 0x0000,
            .FilterMaskIdHigh = 0x0000,
            .FilterMaskIdLow = 0x0000,
            .FilterFIFOAssignment = CAN_FILTER_FIFO0,
            .FilterBank = 0,
            .FilterMode = CAN_FILTERMODE_IDMASK,
            .FilterScale = CAN_FILTERSCALE_32BIT,
            .FilterActivation = CAN_FILTER_ENABLE,
            .SlaveStartFilterBank = 14,
        },
    };
    static const CAN_FilterTypeDef can_t_filters[] = {
        {
            .FilterIdHigh = 0x0000,
            .FilterIdLow = 0x0000,
            .FilterMaskIdHigh = 0x0000,
            .FilterMaskIdLow = 0x0000,
            .FilterFIFOAssignment = CAN_FILTER_FIFO0,
            .FilterBank = 14,
            .FilterMode = CAN_FILTERMODE_IDMASK,
            .FilterScale = CAN_FILTERSCALE_32BIT,
            .FilterActivation = CAN_FILTER_ENABLE,
            .SlaveStartFilterBank = 14,
        },
    };

    rtcan_handle_t *rtcan_handles[] = { &vcu_ptr->rtcan_s, &vcu_ptr->rtcan_t };
    CAN_HandleTypeDef *can_handles[] = { can_s_h, can_t_h };
    const uint32_t rtcan_priorities[] = { vcu_ptr->config_ptr->rtos.rtcan_s_priority,
                                          vcu_ptr->config_ptr->rtos.rtcan_t_priority };
    const CAN_FilterTypeDef *rtcan_filters[] = { can_s_filters, can_t_filters };
    const uint32_t rtcan_filter_counts[] = { 1U, 1U };

    for (uint32_t i = 0; i < 2; i++)
    {
        if (status == STATUS_OK)
        {
            const rtcan_config_t rtcan_config = {
                .thread_priority = rtcan_priorities[i],
                .tx_thread_stack_size = 0U,
                .rx_thread_stack_size = 0U,
                .tx_thread_stack_mem = NULL,
                .rx_thread_stack_mem = NULL,
                .filters = rtcan_filters[i],
                .filter_count = rtcan_filter_counts[i],
            };

            rtcan_status_t rtcan_status =
                rtcan_init(rtcan_handles[i], can_handles[i], &rtcan_config);

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
        status = canbc_init(&vcu_ptr->canbc, &vcu_ptr->rtcan_t, &vcu_ptr->rtcan_s,
                            app_mem_pool, &vcu_ptr->config_ptr->canbc);
    }

    // dash
    if (status == STATUS_OK)
    {
        status = dash_init(&vcu_ptr->dash, app_mem_pool, &vcu_ptr->config_ptr->dash);
    }

    // tick
    if (status == STATUS_OK)
    {
        status = tick_init(&vcu_ptr->tick, &vcu_ptr->canbc, app_mem_pool,
                           &vcu_ptr->config_ptr->tick, &vcu_ptr->config_ptr->apps,
                           &vcu_ptr->config_ptr->bps, &vcu_ptr->config_ptr->ext_inputs);
    }

    // pm100
    if (status == STATUS_OK)
    {
        status = pm100_init(&vcu_ptr->pm100, app_mem_pool, &vcu_ptr->rtcan_t,
                            &vcu_ptr->rtcan_s, &vcu_ptr->config_ptr->pm100);
    }

    // control
    if (status == STATUS_OK)
    {
        status = ctrl_init(&vcu_ptr->ctrl, &vcu_ptr->dash, &vcu_ptr->pm100,
                           &vcu_ptr->tick, &vcu_ptr->remote_ctrl, &vcu_ptr->canbc,
                           &vcu_ptr->fans, &vcu_ptr->wheelspeed, app_mem_pool,
                           &vcu_ptr->config_ptr->ctrl, &vcu_ptr->config_ptr->rtds,
                           &vcu_ptr->config_ptr->torque_map,
                           &vcu_ptr->config_ptr->torque_limiters);
    }

    // remote control
    if (status == STATUS_OK)
    {
        status = remote_ctrl_init(&vcu_ptr->remote_ctrl, &vcu_ptr->canbc, app_mem_pool,
                                  &vcu_ptr->rtcan_s, &vcu_ptr->config_ptr->remote_ctrl);
    }

    // wheelspeed
    if (status == STATUS_OK)
    {
        status = wheelspeed_init(&vcu_ptr->wheelspeed, &vcu_ptr->rtcan_s,
                                 app_mem_pool, &vcu_ptr->config_ptr->wheelspeed);
    }

    // fans
    if (status == STATUS_OK)
    {
        status = fans_init(&vcu_ptr->fans, &vcu_ptr->rtcan_s, app_mem_pool,
                           &vcu_ptr->config_ptr->fans);
    }

    // sd card
    if (status == STATUS_OK)
    {
        status = sd_init(&vcu_ptr->sd, app_mem_pool, &vcu_ptr->config_ptr->sd);
    }

    if (status != STATUS_OK)
        LOG_ERROR("Some services failed to initialise\n");

    return status;
}

/**
 * @brief       Handles CAN transmit mailbox callbacks
 *
 * @param[in]   vcu_ptr VCU instance
 * @param[in]   can_h   CAN handle from callback
 */
status_t vcu_handle_can_tx_mailbox_callback(vcu_context_t *vcu_ptr, CAN_HandleTypeDef *can_h)
{
    // TODO: how to handle CAN C vs CAN S
    rtcan_status_t status = rtcan_handle_tx_mailbox_callback(&vcu_ptr->rtcan_t, can_h);

    if (status != RTCAN_OK)
    {
        // TODO: error
        // vcu_h->err |= VCU_ERROR_PERIPHERAL;
        // Error_Handler();
    }

    status = rtcan_handle_tx_mailbox_callback(&vcu_ptr->rtcan_s, can_h);

    if (status != RTCAN_OK)
    {
        // TODO: error
        // vcu_h->err |= VCU_ERROR_PERIPHERAL;
        // Error_Handler();
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
status_t vcu_handle_can_rx_it(vcu_context_t *vcu_ptr, CAN_HandleTypeDef *can_h, uint32_t rx_fifo)
{
    rtcan_status_t status;

    if (vcu_ptr->rtcan_t.hcan == can_h)
    {
        status = rtcan_handle_rx_it(&vcu_ptr->rtcan_t, can_h, rx_fifo);
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

    LOG_ERROR("CAN error 0x%08lX on %s\n", HAL_CAN_GetError(can_h),
              (can_h == vcu_ptr->rtcan_t.hcan) ? "CAN_T" : "CAN_S");

    if (vcu_ptr->rtcan_t.hcan == can_h)
    {
        status = rtcan_handle_hal_error(&vcu_ptr->rtcan_t, can_h);
    }
    else if (vcu_ptr->rtcan_s.hcan == can_h)
    {
        status = rtcan_handle_hal_error(&vcu_ptr->rtcan_s, can_h);
    }

    if (status != RTCAN_OK)
    {
        // vcu_ptr->err |= VCU_OS_ERROR_PERIPHERAL;

        // Note: Do not call Error_Handler() here as the
        // Entire VCU will crash whenever there is a CAN bus error.
    }

    return STATUS_OK;
}

/**
 * @brief       Handles GPIO EXTI callbacks (e.g. wheelspeed sensors)
 *
 * @param[in]   vcu_ptr     VCU instance
 * @param[in]   gpio_pin    GPIO pin that triggered the interrupt
 */
void vcu_handle_gpio_exti(vcu_context_t *vcu_ptr, uint16_t gpio_pin)
{
    wheelspeed_handle_exti(&vcu_ptr->wheelspeed, gpio_pin);
}
