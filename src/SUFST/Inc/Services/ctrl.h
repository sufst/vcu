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

#include "canbc.h"
#include "config.h"
#include "dash.h"
#include "status.h"

/*
 * error codes
 */
#define CTRL_ERROR_NONE              0x0
#define CTRL_ERROR_INIT              0x1
#define CTRL_ERROR_TS_READY_TIMEOUT  0x2
#define CTRL_ERROR_PRECHARGE_TIMEOUT 0x4

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
} ctrl_state_t;

/**
 * @brief   Control service context
 */
typedef struct
{
    ctrl_state_t state;              // state machine state
    TX_THREAD thread;                // service thread
    dash_context_t* dash_ptr;        // dash service
    canbc_context_t* canbc_ptr;      // CANBC service
    const config_ctrl_t* config_ptr; // config
    uint8_t error;                   // error code
} ctrl_context_t;

/*
 * public functions
 */
status_t ctrl_init(ctrl_context_t* ctrl_ptr,
                   dash_context_t* dash_ptr,
                   canbc_context_t* canbc_ptr,
                   TX_BYTE_POOL* stack_pool_ptr,
                   const config_ctrl_t* config_ptr);

#endif
