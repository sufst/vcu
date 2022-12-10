/******************************************************************************
 * @file    pm100.h
 * @author  Tim Brewis (@t-bre)
 * @brief   Driver for the PM100DZ inverter
 *****************************************************************************/

#ifndef PM100_H
#define PM100_H

#include "rtcan.h"

/**
 * @brief   PM100DZ inverter context
 */
typedef struct {

    /**
     * @brief   RTCAN instance for communications with inverter
     */
    rtcan_handle_t* rtcan_h;

    /**
     * @brief   Inverter state
     */
    enum {
        PM100_STATE_DISABLED,
        PM100_STATE_LOCKOUT,
        PM100_STATE_ENABLED,
        PM100_STATE_FAULT
    } state;

} pm100_handle_t;

/*
 * function prototypes
 */
void pm100_init(pm100_handle_t* pm100_h, rtcan_handle_t* rtcan_h);
void pm100_enable(pm100_handle_t* pm100_h);
void pm100_disable(pm100_handle_t* pm100_h);
void pm100_request_torque(pm100_handle_t* pm100_h);
void pm100_handle_broadcast_msg(pm100_handle_t* pm100_h, rtcan_msg_t* msg_ptr);

#endif