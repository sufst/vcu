/***************************************************************************
 * @file    canbc.h
 * @author  Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief   Broadcasts state information to a CAN bus using the RTCAN
 *          service
 ***************************************************************************/

#ifndef CANBC_H
#define CANBC_H

#include <can.h>
#include <rtcan.h>
#include <stdint.h>
#include <tx_api.h>

#include "config.h"
#include "status.h"

/**
 * @brief   Stores system states which will be broadcast to the CAN bus
 *
 * @details This is a COPY of specific module states which are useful to
 *          monitor with the datalogger / telemetry systems. Modules are
 *          responsible for updating the broadcast states when their
 *          corresponding states change.
 */
typedef struct
{
    // driver inputs
    uint16_t apps_reading;
    uint16_t bps_reading;
    uint32_t torque_request;

    // state
    uint16_t ctrl_state;

    // module errors
    uint8_t ctrl_error;

} canbc_states_t;

/**
 * @brief   CAN broadcasting service context
 */
typedef struct
{
    TX_THREAD thread;                 // service thread
    TX_MUTEX state_mutex;             // mutex for locking broadcast states
    rtcan_handle_t* rtcan_h;          // RTCAN instance to broadcast on
    canbc_states_t states;            // broadcasting states
    uint16_t rolling_counter;         // counts number of broadcasts
    const config_canbc_t* config_ptr; // configuration

} canbc_context_t;

/*
 * public functions
 */
status_t canbc_init(canbc_context_t* canbc_h,
                    rtcan_handle_t* rtcan_h,
                    TX_BYTE_POOL* stack_pool_ptr,
                    const config_canbc_t* config_ptr);

canbc_states_t* canbc_lock_state(canbc_context_t* canbc_h, uint32_t timeout);

void canbc_unlock_state(canbc_context_t* canbc_h);

#endif