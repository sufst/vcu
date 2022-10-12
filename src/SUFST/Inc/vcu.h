/******************************************************************************
 * @file    vcu.h
 * @author  Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief   Top level VCU implementation
 * @details This encapsulates the entire VCU state as much as possible
 *****************************************************************************/

#ifndef VCU_H
#define VCU_H

#include <stdint.h>

#include "tx_api.h"

#include "can.h"
#include "canbc.h"
#include "rtcan.h"
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

    /**
     * @brief   RTCAN service for the sensors CAN bus
     */
    rtcan_handle_t rtcan_s;

    /**
     * @brief   CAN broadcasting service for VCU state
     *
     * @details Broadcasts to the sensors CAN bus using the RTCAN service
     */
    canbc_handle_t canbc;

    /**
     * @brief   Tractive system controller
     */
    ts_ctrl_handle_t ts_ctrl;

    /**
     * @brief   Initialisation thread
     */
    TX_THREAD init_thread;

    /**
     * @brief   Current error code
     */
    uint32_t err;

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
                      CAN_HandleTypeDef* can_s_h,
                      TX_BYTE_POOL* app_mem_pool);

vcu_status_t vcu_handle_can_tx_mailbox_callback(vcu_handle_t* vcu_h,
                                                CAN_HandleTypeDef* can_h);

#endif