/******************************************************************************
 * @file    vcu.h
 * @author  Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief   Top level VCU implementation
 * @details This encapsulates the entire VCU state as much as possible
 *****************************************************************************/

#ifndef VCU_H
#define VCU_H

#include <can.h>
#include <rtcan.h>
#include <stdint.h>
#include <tx_api.h>

#include "canbc.h"
#include "config.h"
#include "ctrl.h"
#include "dash.h"
#include "heartbeat.h"
#include "log.h"
#include "pm100.h"
#include "remote_ctrl.h"
#include "status.h"
#include "tick.h"

/**
 * @brief       VCU context
 *
 * @details     Contains the entire state of the VCU and all module instances
 */
typedef struct
{
    rtcan_handle_t rtcan_s; // RTCAN service for sensors CAN bus
    rtcan_handle_t rtcan_c; // RTCAN service for critical systems CAN bus
    canbc_context_t canbc;  // CAN broadcasting service instance
    dash_context_t dash;    // dash service
    ctrl_context_t ctrl;    // control service
    pm100_context_t pm100;  // PM100 service
    tick_context_t tick;
    remote_ctrl_context_t remote_ctrl;
    heartbeat_context_t heartbeat; // heartbeat service
    log_context_t log;             // logging service
    uint32_t err;                  // current error code
    const config_t* config_ptr;    // pointer to global VCU configuration

} vcu_context_t;

/*
 * public functions
 */

status_t vcu_init(vcu_context_t* vcu_ptr,
                  CAN_HandleTypeDef* can_c_h,
                  CAN_HandleTypeDef* can_s_h,
                  TX_BYTE_POOL* app_mem_pool,
                  const config_t* config_ptr);

status_t vcu_handle_can_tx_mailbox_callback(vcu_context_t* vcu_ptr,
                                            CAN_HandleTypeDef* can_h);

status_t vcu_handle_can_rx_it(vcu_context_t* vcu_ptr,
                              CAN_HandleTypeDef* can_h,
                              uint32_t rx_fifo);

status_t vcu_handle_can_err(vcu_context_t* vcu_ptr, CAN_HandleTypeDef* can_h);

#endif
