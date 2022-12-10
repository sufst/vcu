/******************************************************************************
 * @file    pm100.h
 * @author  Tim Brewis (@t-bre)
 * @brief   Driver for the PM100DZ inverter
 *****************************************************************************/

#ifndef PM100_H
#define PM100_H

#include <stdbool.h>
#include <stdint.h>

#include "rtcan.h"

#include "status.h"

/*
 * error codes
 */
#define PM100_ERROR_NONE  0x00000000U // no error
#define PM100_ERROR_INIT  0x00000001U // failed to initialise
#define PM100_ERROR_FAULT 0x00000002U // fault reported by PM100

/**
 * @brief   PM100DZ inverter context
 */
typedef struct
{

    /**
     * @brief   RTCAN instance for communications with inverter
     */
    rtcan_handle_t* rtcan_h;

    /**
     * @brief   Waiting on lockout flag
     */
    bool waiting_on_lockout;

    /**
     * @brief   Lockout semaphore
     *
     * @details This is incremented when
     */
    TX_SEMAPHORE lockout_sem;

    /**
     * @brief   Current error code
     */
    uint32_t err;

} pm100_handle_t;

/*
 * function prototypes
 */
status_t pm100_init(pm100_handle_t* pm100_h, rtcan_handle_t* rtcan_h);
status_t pm100_enable(pm100_handle_t* pm100_h);
status_t pm100_disable(pm100_handle_t* pm100_h);
status_t pm100_request_torque(pm100_handle_t* pm100_h, uint32_t torque);
status_t pm100_handle_broadcast_msg(pm100_handle_t* pm100_h,
                                    rtcan_msg_t* msg_ptr);

#endif