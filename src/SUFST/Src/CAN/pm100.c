/******************************************************************************
 * @file    pm100.c
 * @author  Tim Brewis (@t-bre)
 * @details Implementation based on information in the "Cascadia Motion CAN
 *          Protocol" v5.9 document
 *****************************************************************************/

#include "pm100.h"

#define PM100_MAX_TORQUE_REQUEST         0x7FFF
#define PM100_DIRECTION_FORWARD          0x1
#define PM100_DIRECTION_REVERSE          0x0

#define PM100_INVERTER_DISABLE           0x0
#define PM100_INVERTER_ENABLE            0x1
#define PM100_INVERTER_DISCHARGE_DISABLE 0x0
#define PM100_INVERTER_DISCHARGE_ENABLE  0x2
#define PM100_SPEED_MODE_DISABLE         0x0
#define PM100_SPEED_MODE_ENABLE          0x4

#define ADD_ERROR_IF(cond, error, inst) \
    if (cond)                           \
    {                                   \
        inst->err |= error;             \
    }

static bool no_errors(pm100_handle_t* pm100_h);
static status_t create_status(pm100_handle_t* pm100_h);

/*
 * types
 */

/**
 * @brief       Initialises the inverter context
 *
 * @param[in]   pm100_h     PM100 handle
 * @param[in]   rtcan_h     RTCAN handle for inverter communications
 */
status_t pm100_init(pm100_handle_t* pm100_h, rtcan_handle_t* rtcan_h)
{
    pm100_h->rtcan_h = rtcan_h;
    pm100_h->waiting_on_lockout = true; // assume lockout state
    pm100_h->err = PM100_ERROR_NONE;

    // setup RTCAN subscriptions
    if (no_errors(pm100_h))
    {
        UINT status = tx_queue_create(&pm100_h->can_rx_queue,
                                      "PM100 CAN Receive Queue",
                                      TX_1_ULONG,
                                      pm100_h->can_rx_queue_mem,
                                      sizeof(pm100_h->can_rx_queue_mem));

        ADD_ERROR_IF(status != STATUS_OK, PM100_ERROR_INIT, pm100_h);
    }

    if (no_errors(pm100_h))
    {
        rtcan_status_t status
            = rtcan_subscribe(pm100_h->rtcan_h,
                              CAN_DATABASE_PM100_INTERNAL_STATES_FRAME_ID,
                              &pm100_h->can_rx_queue);

        ADD_ERROR_IF(status != RTCAN_OK, PM100_ERROR_INIT, pm100_h);
    }

    return create_status(pm100_h);
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
    // // disable inverter
    // status_t status = pm100_disable(pm100_h);

    // // wait for lockout
    // if (status == STATUS_OK)
    // {
    //     pm100_h->waiting_on_lockout = true;

    //     status = (tx_semaphore_get(&pm100_h->lockout_sem, TX_WAIT_FOREVER)
    //               == TX_SUCCESS)
    //                  ? STATUS_OK
    //                  : STATUS_ERROR;

    //     pm100_h->waiting_on_lockout = false;
    // }

    // // send a zero torque request
    // if (status == STATUS_OK)
    // {
    //     status = pm100_request_torque(pm100_h, 0);
    // }

    return STATUS_OK;
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
    // rtcan_msg_t msg = {.identifier = CAN_ID_PM100DZ_COMMAND,
    //                    .data = {0, 0, 0, 0, 0, 0, 0, 0},
    //                    .length = 8};

    // rtcan_status_t status = rtcan_transmit(pm100_h->rtcan_h, &msg);

    return STATUS_OK;
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
    // status_t status
    //     = (torque <= PM100_MAX_TORQUE_REQUEST) ? STATUS_OK : STATUS_ERROR;

    // if (status == STATUS_OK)
    // {
    //     rtcan_msg_t msg = {
    //         .identifier = CAN_ID_PM100DZ_COMMAND,
    //         .data = {
    //             // torque request
    //             (torque & 0x00FF),
    //             (torque & 0xFF00) >> 8,
    //             // speed command
    //             0,
    //             0,
    //             // direction
    //             PM100_DIRECTION_FORWARD,
    //             // inverter and speed control
    //             PM100_INVERTER_ENABLE
    //             | PM100_INVERTER_DISCHARGE_DISABLE
    //             | PM100_SPEED_MODE_DISABLE,
    //             // torque limit
    //             (PM100_MAX_TORQUE_REQUEST & 0x00FF),
    //             (PM100_MAX_TORQUE_REQUEST & 0xFF00) >> 8
    //         },
    //         .length = 8
    //     };

    //     status = (rtcan_transmit(pm100_h->rtcan_h, &msg) == RTCAN_OK)
    //                  ? STATUS_OK
    //                  : STATUS_ERROR;
    // }

    return STATUS_OK;
}

/**
 * @brief       Processes all incoming broadcast messages from the PM100
 *
 * @param[in]   pm100_h     PM100 handle
 */
status_t pm100_process_broadcast_msgs(pm100_handle_t* pm100_h)
{
    rtcan_msg_t* msg_ptr = NULL;

    while (
        tx_queue_receive(&pm100_h->can_rx_queue, (void*) &msg_ptr, TX_NO_WAIT))
    {
        switch (msg_ptr->identifier)
        {
        case CAN_DATABASE_PM100_INTERNAL_STATES_FRAME_ID:
            break;

        default:
            break;
        }
    }

    // TODO: are any errors possible here?
    return STATUS_OK;
}

/**
 * @brief       Returns true if the PM100 instance has encountered an
 *              error
 *
 * @param[in]   pm100_h     PM100 handle
 */
static bool no_errors(pm100_handle_t* pm100_h)
{
    return (pm100_h->err == PM100_ERROR_NONE);
}

/**
 * @brief       Create a status code based on the current error state
 *
 * @param[in]   pm100_h   PM100 handle
 */
static status_t create_status(pm100_handle_t* pm100_h)
{
    return (no_errors(pm100_h)) ? STATUS_OK : STATUS_ERROR;
}
