/*****************************************************************************
 * @file   driver_control.h
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief  Driver control
 ****************************************************************************/

#ifndef DRIVER_CONTROL_H
#define DRIVER_CONTROL_H

#include <stdint.h>
#include <tx_api.h>

#include "canbc.h"
#include "config.h"
#include "ts_control.h"

/*
 * error codes
 */
#define DRIVER_CTRL_ERROR_NONE     0x00000000U // no error
#define DRIVER_CTRL_ERROR_INIT     0x00000001U // failed to start service
#define DRIVER_CTRL_ERROR_INTERNAL 0x80000000U // internal error

/**
 * @brief   Driver control handle
 */
typedef struct
{

    /**
     * @brief   Input processing thread
     */
    TX_THREAD thread;

    /**
     * @brief   Sampling tick timer
     *
     * @details This event causes the thread to wake which will result in
     *          a whole bunch of other processing to act on the driver input
     */
    TX_TIMER tick_timer;

    /**
     * @brief   TS controller handle
     *
     * @details Used to pass inputs to controller
     */
    ts_ctrl_handle_t* ts_ctrl_h;

    /**
     * @brief   State mutex for CAN broadcast
     */
    TX_MUTEX state_mutex;

    /**
     * @brief   Driver inputs state
     */
    ts_ctrl_input_t ts_inputs;

    /**
     * @brief   CAN broadcaster instance
     */
    canbc_handle_t* canbc_h;

    /**
     * @brief   Current error code
     */
    uint32_t err;

    /**
     * @brief   Configuration
     */
    const config_ts_activation_t* ts_activation_config_ptr;

} dc_handle_t;

/**
 * @brief   Driver controls status
 */
typedef enum
{
    DRIVER_CTRL_OK,
    DRIVER_CTRL_ERROR
} driver_ctrl_status_t;

/*
 * function prototypes
 */
driver_ctrl_status_t
driver_ctrl_init(dc_handle_t* dc_h,
                 ts_ctrl_handle_t* ts_ctrl_h,
                 canbc_handle_t* canbc_h,
                 TX_BYTE_POOL* stack_pool_ptr,
                 const config_ts_activation_t* ts_activation_config);

driver_ctrl_status_t driver_ctrl_start(dc_handle_t* dc_h);

#endif
