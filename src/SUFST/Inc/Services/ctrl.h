/*****************************************************************************
 * @file    ctrl.h
 * @author  Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief   Control service
 * @details This service is responsible for controlling VCU based on driver
 *          inputs
 ****************************************************************************/

#ifndef CTRL_H
#define CTRL_H

#include <stdint.h>
#include <tx_api.h>

#include "apps.h"
#include "canbc.h"
#include "config.h"
#include "dash.h"
#include "status.h"

/*
 * error codes
 */
#define CTRL_ERROR_NONE               0x00
#define CTRL_ERROR_INIT               0x01 // service failed to initialise
#define CTRL_ERROR_TS_READY_TIMEOUT   0x02 // TS ready from TRC timed out
#define CTRL_ERROR_PRECHARGE_TIMEOUT  0x04 // precharge timed out
#define CTRL_ERROR_TRC_RUN_FAULT      0x08 // TRC faulted at runtime
#define CTRL_ERROR_INVERTER_RUN_FAULT 0x10 // inverter faulted at runtime

/**
 * @brief   Control state
 */
typedef enum
{
    CTRL_STATE_TS_OFF,
    CTRL_STATE_TS_READY_WAIT,
    CTRL_STATE_PRECHARGE_WAIT,
    CTRL_STATE_R2D_WAIT,
    CTRL_STATE_TS_ON,
    CTRL_STATE_TS_ACTIVATION_FAILURE,
    CTRL_STATE_TS_RUN_FAULT,
    CTRL_STATE_SCS_FAULT
} ctrl_state_t;

/**
 * @brief   Control service context
 */
typedef struct
{
    ctrl_state_t state;                   // state machine state
    TX_THREAD thread;                     // service thread
    dash_context_t* dash_ptr;             // dash service
    canbc_context_t* canbc_ptr;           // CANBC service
    apps_context_t apps;                  // APPS interface instance
    uint16_t apps_reading;                // lastest APPS reading
    const config_ctrl_t* config_ptr;      // config
    const config_rtds_t* rtds_config_ptr; // RTDS config
    uint8_t error;                        // error code
} ctrl_context_t;

/*
 * public functions
 */
status_t ctrl_init(ctrl_context_t* ctrl_ptr,
                   dash_context_t* dash_ptr,
                   canbc_context_t* canbc_ptr,
                   TX_BYTE_POOL* stack_pool_ptr,
                   const config_ctrl_t* config_ptr,
                   const config_apps_t* apps_config_ptr,
                   const config_rtds_t* rtds_config_ptr);

#endif
