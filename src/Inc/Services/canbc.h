/***************************************************************************
 * @file    canbc.h
 * @author  Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief   Broadcasts state information to a CAN bus using the RTCAN
 *          service
 ***************************************************************************/

#ifndef CANBC_H
#define CANBC_H

#include "log.h"
#include <FreeRTOS.h>
#include <can.h>
#include <can_s.h>
#include <can_t.h>
#include <rtcan.h>
#include <semphr.h>
#include <stdint.h>
#include <task.h>

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
    struct can_s_vcu_sensors_t sensors;
    struct can_s_vcu_sensors_raw_t sensors_raw;
    struct can_s_vcu_temps_t temps;
    struct can_t_vcu_state_t state;
    struct can_t_vcu_error_t errors;
    struct can_t_vcu_launch_ctrl_t launch_ctrl;
    struct can_t_vcu_pdm_t pdm;
} canbc_states_t;

/**
 * @brief   CAN broadcasting service context
 */
typedef struct
{
    TaskHandle_t thread;              // service thread
    SemaphoreHandle_t state_mutex;    // mutex for locking broadcast states
    StaticSemaphore_t state_mutex_cb; // state mutex control block
    rtcan_handle_t *rtcan_t_h;        // RTCAN instance for the tractive bus
    rtcan_handle_t *rtcan_s_h;        // RTCAN instance for the sensor bus
    canbc_states_t states;            // broadcasting states
    const config_canbc_t *config_ptr; // configuration

    uint8_t rtcan1_error; // sticky flag: CAN1 (CAN-S) transmit failure
    uint8_t rtcan2_error; // sticky flag: CAN2 (CAN-T) transmit failure
    uint8_t canbc_error; // sticky flag: canbc service failure (e.g. lock timeout)
    TickType_t tx_error_log_tick_t; // rate limit of the CAN-T transmit failure log
    TickType_t tx_error_log_tick_s; // rate limit of the CAN-S transmit failure log

} canbc_context_t;

/*
 * public functions
 */
status_t canbc_init(canbc_context_t *canbc_h,
                    rtcan_handle_t *rtcan_t_h,
                    rtcan_handle_t *rtcan_s_h,
                    const config_canbc_t *config_ptr);

canbc_states_t *canbc_lock_state(canbc_context_t *canbc_h, TickType_t timeout);

void canbc_unlock_state(canbc_context_t *canbc_h);

#endif
