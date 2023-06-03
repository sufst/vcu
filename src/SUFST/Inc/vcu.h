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
#include "ts_control.h"

/*
 * error codes
 */
#define VCU_ERROR_NONE       0x00000000U // no error
#define VCU_ERROR_INIT       0x00000001U // failed to initialise VCU
#define VCU_ERROR_PERIPHERAL 0x00000002U // runtime error with peripheral

/**
 * @brief   Contains the entire state of the VCU and handles to any peripherals
 */
typedef struct
{
    rtcan_handle_t rtcan_s; // RTCAN service for sensors CAN bus
    rtcan_handle_t rtcan_c; // RTCAN service for critical systems CAN bus
    canbc_handle_t canbc;   // CAN broadcasting service instance

    dash_context_t dash; // dash service
    ctrl_context_t ctrl; // control service

    uint32_t err;               // current error code
    const config_t* config_ptr; // pointer to global VCU configuration

} vcu_handle_t;

/**
 * @brief   VCU status
 */
typedef enum
{
    VCU_OK,
    VCU_ERROR
} vcu_status_t;

/*
 * function prototypes
 */

vcu_status_t vcu_init(vcu_handle_t* vcu_h,
                      CAN_HandleTypeDef* can_c_h,
                      CAN_HandleTypeDef* can_s_h,
                      TX_BYTE_POOL* app_mem_pool,
                      const config_t* config_ptr);

vcu_status_t vcu_handle_can_tx_mailbox_callback(vcu_handle_t* vcu_h,
                                                CAN_HandleTypeDef* can_h);

vcu_status_t vcu_handle_can_rx_it(vcu_handle_t* vcu_h,
                                  CAN_HandleTypeDef* can_h,
                                  uint32_t rx_fifo);

vcu_status_t vcu_handle_can_err(vcu_handle_t* vcu_h, CAN_HandleTypeDef* can_h);

#endif