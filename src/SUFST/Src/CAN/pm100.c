/******************************************************************************
 * @file    pm100.c
 * @author  Tim Brewis (@t-bre)
 *****************************************************************************/

#include "pm100.h"

/**
 * @brief       Initialises the inverter context
 * 
 * @param[in]   pm100_h     PM100 handle
 * @param[in]   rtcan_h     RTCAN handle for inverter communications
 */
void pm100_init(pm100_handle_t* pm100_h, rtcan_handle_t* rtcan_h)
{
    pm100_h->rtcan_h = rtcan_h;
    pm100_h->state = PM100_STATE_DISABLED;

    // TODO: subscribe to RTCAN messages for inverter broadcasts
}

/**
 * @brief       Enables the inverter
 * 
 * @param[in]   pm100_h 
 */
void pm100_enable(pm100_handle_t* pm100_h)
{
    // TODO: implementation
    // TODO: disable lockout here
}


/**
 * @brief       Disables the inverter
 * 
 * @param[in]   pm100_h 
 */
void pm100_disable(pm100_handle_t* pm100_h)
{
    // TODO: implementation
}

/**
 * @brief       Sends a torque request to the inverter
 * 
 * @param[in]   pm100_h     PM100 handle
 */
void pm100_request_torque(pm100_handle_t* pm100_h)
{
    // TODO: implementation
}

/**
 * @brief       Handles broadcast messages received from the inverter 
 * 
 * @param[in]   pm100_h     PM100 handle
 * @param[in]   msg_ptr     Pointer to broadcast message ID
 */
void pm100_handle_broadcast_msg(pm100_handle_t* pm100_h, rtcan_msg_t* msg_ptr)
{
    // TODO: implementation
}