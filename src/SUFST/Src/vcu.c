/******************************************************************************
 * @file    vcu.h
 * @author  Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief   Top level VCU implementation
 *****************************************************************************/

#include "vcu.h"

#include <stdbool.h>

#include "config.h"

/*
 * internal function prototypes
 */
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