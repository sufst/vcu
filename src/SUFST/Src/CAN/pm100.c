/******************************************************************************
 * @file    pm100.c
 * @author  Tim Brewis (@t-bre)
 *****************************************************************************/

#include "pm100.h"

#define PM100_MAX_TORQUE_REQUEST    32767

/**
 * @brief       Initialises the inverter context
 * 
 * @param[in]   pm100_h     PM100 handle
 * @param[in]   rtcan_h     RTCAN handle for inverter communications
 */
status_t pm100_init(pm100_handle_t* pm100_h, rtcan_handle_t* rtcan_h)
{
    pm100_h->rtcan_h = rtcan_h;
    pm100_h->state = PM100_STATE_DISABLED;
    pm100_h->err = PM100_ERROR_NONE;

    // TODO: subscribe to RTCAN messages for inverter broadcasts
    return STATUS_OK;
}

/**
 * @brief       Enables the inverter
 * 
 * @param[in]   pm100_h 
 */
status_t pm100_enable(pm100_handle_t* pm100_h)
{
    // TODO: implementation
    // TODO: disable lockout here
    return STATUS_OK;
}


/**
 * @brief       Disables the inverter
 * 
 * @param[in]   pm100_h 
 */
status_t pm100_disable(pm100_handle_t* pm100_h)
{
    // TODO: implementation
    return STATUS_OK;
}

/**
 * @brief       Sends a torque request to the inverter
 * 
 * @param[in]   pm100_h     PM100 handle
 * @param[in]   torque      Requested torque in range 0 to 32767
 */
status_t pm100_request_torque(pm100_handle_t* pm100_h, uint32_t torque)
{
    status_t status = (torque <= PM100_MAX_TORQUE_REQUEST);

    // TODO: implementation
    
    return status;
}

/**
 * @brief       Handles broadcast messages received from the inverter 
 * 
 * @param[in]   pm100_h     PM100 handle
 * @param[in]   msg_ptr     Pointer to broadcast message ID
 */
status_t pm100_handle_broadcast_msg(pm100_handle_t* pm100_h, rtcan_msg_t* msg_ptr)
{
    // TODO: implementation
    return STATUS_OK;
}