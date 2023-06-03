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

    // VCU internal states
    struct
    {
        uint8_t r2d : 1;
        uint8_t shutdown : 1;
        uint8_t UNUSED1 : 6;
    } vcu_state;

} canbc_states_t;

/**
 * @brief   CAN broadcasting service context
 */
typedef struct
{

    TX_THREAD thread;
    TX_MUTEX state_mutex;

    rtcan_handle_t* rtcan_h;
    uint32_t bc_period;

    canbc_states_t states;
    uint16_t rolling_counter;

} canbc_handle_t;

/*
 * public functions
 */
void canbc_init(canbc_handle_t* canbc_h,
                rtcan_handle_t* rtcan_h,
                uint32_t period,
                uint32_t priority,
                TX_BYTE_POOL* stack_pool_ptr);

canbc_states_t* canbc_lock_state(canbc_handle_t* canbc_h, uint32_t timeout);

void canbc_unlock_state(canbc_handle_t* canbc_h);

#endif