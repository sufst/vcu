/******************************************************************************
 * @file    pm100.c
 * @author  Tim Brewis (@t-bre)
 * @details Implementation based on information in the "Cascadia Motion CAN
 *          Protocol" v5.9 document
 *****************************************************************************/

#include "pm100.h"

#define PM100DZ_GEN              5    // TODO: double check this
#define PM100DZ_FIRMWARE_VERSION 2000 // TODO: this is unknown
#include "pm100dz_def.h"

#define PM100_MAX_TORQUE_REQUEST         0x7FFF
#define PM100_DIRECTION_FORWARD          0x1
#define PM100_DIRECTION_REVERSE          0x0

#define PM100_INVERTER_DISABLE           0x0
#define PM100_INVERTER_ENABLE            0x1
#define PM100_INVERTER_DISCHARGE_DISABLE 0x0
#define PM100_INVERTER_DISCHARGE_ENABLE  0x2
#define PM100_SPEED_MODE_DISABLE         0x0
#define PM100_SPEED_MODE_ENABLE          0x4

#define PM100_LOCKOUT_SEM_NAME           "PM100 Lockout Semaphore"

/**
 * @brief       Initialises the inverter context
 *
 * @param[in]   pm100_h     PM100 handle
 * @param[in]   rtcan_h     RTCAN handle for inverter communications
 */
status_t pm100_init(pm100_handle_t* pm100_h, rtcan_handle_t* rtcan_h)
{
    pm100_h->rtcan_h = rtcan_h;
    pm100_h->waiting_on_lockout = false;
    pm100_h->err = PM100_ERROR_NONE;

    // create lockout semaphore
    status_t status
        = (tx_semaphore_create(&pm100_h->lockout_sem, PM100_LOCKOUT_SEM_NAME, 0)
           == TX_SUCCESS)
              ? STATUS_OK
              : STATUS_ERROR;

    // TODO: subscribe to RTCAN messages for inverter broadcasts
    return status;
}

/**
 * @brief       Enables the inverter, suspending the calling thread until
 *              the process is complete
 *
 * @details     The inverter initialises in the "lockout" state. To enable it,
 *              a disable command must first be sent.
 *
 * @see         "Cascadia Motion CAN Protocol" v5.9 p.35
 *
 * @param[in]   pm100_h     PM100 handle
 */
status_t pm100_enable(pm100_handle_t* pm100_h)
{
    // disable inverter
    status_t status = pm100_disable(pm100_h);

    // wait for lockout
    if (status == STATUS_OK)
    {
        pm100_h->waiting_on_lockout = true;

        status = (tx_semaphore_get(&pm100_h->lockout_sem, TX_WAIT_FOREVER)
                  == TX_SUCCESS)
                     ? STATUS_OK
                     : STATUS_ERROR;

        pm100_h->waiting_on_lockout = false;
    }

    // send a zero torque request
    if (status == STATUS_OK)
    {
        status = pm100_request_torque(pm100_h, 0);
    }

    return status;
}

/**
 * @brief       Disables the inverter
 *
 * @details     Disable command consists of all zeros in data field.
 *
 * @see         "Cascadia Motion CAN Protocol" v5.9 p.35
 *
 * @param[in]   pm100_h     PM100 handle
 */
status_t pm100_disable(pm100_handle_t* pm100_h)
{
    rtcan_msg_t msg = {.identifier = CAN_ID_PM100DZ_COMMAND,
                       .data = {0, 0, 0, 0, 0, 0, 0, 0},
                       .length = 8};

    rtcan_status_t status = rtcan_transmit(pm100_h->rtcan_h, &msg);

    return (status == RTCAN_OK) ? STATUS_OK : STATUS_ERROR;
}

/**
 * @brief       Sends a torque request to the inverter
 *
 * @see         "Cascadia Motion CAN Protocol" v5.9 p.31
 *
 * @param[in]   pm100_h     PM100 handle
 * @param[in]   torque      Requested torque in range 0 to 32767
 */
status_t pm100_request_torque(pm100_handle_t* pm100_h, uint32_t torque)
{
    status_t status
        = (torque <= PM100_MAX_TORQUE_REQUEST) ? STATUS_OK : STATUS_ERROR;

    if (status == STATUS_OK)
    {
        rtcan_msg_t msg = {
            .identifier = CAN_ID_PM100DZ_COMMAND,
            .data = {
                // torque request
                (torque & 0x00FF),
                (torque & 0xFF00) >> 8,
                // speed command
                0,
                0, 
                // direction
                PM100_DIRECTION_FORWARD,
                // inverter and speed control
                PM100_INVERTER_ENABLE 
                | PM100_INVERTER_DISCHARGE_DISABLE 
                | PM100_SPEED_MODE_DISABLE,
                // torque limit
                (PM100_MAX_TORQUE_REQUEST & 0x00FF),
                (PM100_MAX_TORQUE_REQUEST & 0xFF00) >> 8
            },
            .length = 8
        };

        status = (rtcan_transmit(pm100_h->rtcan_h, &msg) == RTCAN_OK)
                     ? STATUS_OK
                     : STATUS_ERROR;
    }

    return status;
}

/**
 * @brief       Handles broadcast messages received from the inverter
 *
 * @param[in]   pm100_h     PM100 handle
 * @param[in]   msg_ptr     Pointer to broadcast message ID
 */
status_t pm100_handle_broadcast_msg(pm100_handle_t* pm100_h,
                                    rtcan_msg_t* msg_ptr)
{
    status_t status = STATUS_OK;

    // handle lockout
    if (msg_ptr->identifier == CAN_ID_PM100DZ_INTERNAL_STATES)
    {
        if (pm100_h->waiting_on_lockout)
        {
            pm100dz_internal_states_t* state
                = (pm100dz_internal_states_t*) msg_ptr;

            if (state->lockout_enabled == 0)
            {
                status = (tx_semaphore_put(&pm100_h->lockout_sem) == TX_SUCCESS)
                             ? STATUS_OK
                             : STATUS_ERROR;
            }
        }
    }

    // TODO: check for faults

    return status;
}