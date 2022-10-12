/***************************************************************************
 * @file   ts_control.h
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief  Tractive System (TS) controller
 ***************************************************************************/

#ifndef TS_CONTROL_H
#define TS_CONTROL_H

#include <stdint.h>

#include "tx_api.h"

/*
 * error codes
 */
#define TS_CTRL_ERROR_NONE     0x00000000U // no error
#define TS_CTRL_ERROR_INIT     0x00000001U // failed to start service
#define TS_CTRL_ERROR_INTERNAL 0x80000000U // internal error

/**
 * @brief   TS control input
 *
 * @note    This must have a size that is a multiple of ULONG for use in
 *          input queue
 */
typedef struct
{

    /**
     * @brief   Accelerator pedal pressure
     */
    uint32_t accel_pressure;

    /**
     * @brief   Brake pressure
     */
    uint32_t brake_pressure;

} ts_ctrl_input_t;

/**
 * @brief   TS torque request
 */
typedef struct
{
    uint32_t torque;
} ts_torque_request_t;

/*
 * queue sizing constants
 */
#define TS_CTRL_INPUT_QUEUE_ITEM_SIZE (sizeof(ts_ctrl_input_t) / sizeof(ULONG))

#define TS_CTRL_INPUT_QUEUE_LENGTH    1 // multiple inputs queued is bad!

#define TS_CTRL_INPUT_QUEUE_SIZE \
    (TS_CTRL_INPUT_QUEUE_ITEM_SIZE * TS_CTRL_INPUT_QUEUE_LENGTH)

/**
 * @brief   TS control handle
 */
typedef struct
{

    /**
     * @brief   Controller processing thread
     */
    TX_THREAD thread;

    /**
     * @brief   Control input queue
     */
    TX_QUEUE input_queue;

    /**
     * @brief   Memory for control input queue
     *
     */
    ULONG input_queue_mem[TS_CTRL_INPUT_QUEUE_SIZE];

    /**
     * @brief   Current error code
     */
    uint32_t err;

} ts_ctrl_handle_t;

/**
 * @brief   TS control status
 */
typedef enum
{
    TS_CTRL_OK,
    TS_CTRL_ERROR
} ts_ctrl_status_t;

/*
 * function prototypes
 */

ts_ctrl_status_t ts_ctrl_init(ts_ctrl_handle_t* ts_ctrl_h,
                              TX_BYTE_POOL* stack_pool_ptr);

ts_ctrl_status_t ts_ctrl_start(ts_ctrl_handle_t* ts_ctrl_h);

ts_ctrl_status_t ts_ctrl_input_send(ts_ctrl_handle_t* ts_ctrl_h,
                                    ts_ctrl_input_t* input_ptr);

#endif
