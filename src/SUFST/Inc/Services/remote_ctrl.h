/*****************************************************************************
 * @file    remote_ctrl.h
 * @author  Dmytro Avdieienko (@Avdieienko, da3e22@soton.ac.uk)
 * @brief   Remote control service
 * @details Thread-safe remote control service implementation intended for use on the dyno
 ****************************************************************************/

#ifndef REMOTE_CTRL_H
#define REMOTE_CTRL_H

#include <stdint.h>
#include <tx_api.h>
#include <can_s.h>
#include <rtcan.h>

#include "config.h"
#include "log.h"
#include "status.h"
#include "canbc.h"
#include "bps.h"
#include "apps.h"

#define REMOTE_CTRL_RX_QUEUE_SIZE 2 // 2 items (8 bytes)

typedef struct
{
    TX_THREAD thread;
    TX_MUTEX sensor_mutex;
    const config_remote_ctrl_t *config_ptr;
    canbc_context_t *canbc_ptr;
    rtcan_handle_t *rtcan_s_ptr;
    TX_QUEUE can_rx_queue;
    ULONG can_rx_queue_mem[REMOTE_CTRL_RX_QUEUE_SIZE];

    struct can_s_vcu_simulation_t requests;
    bool brakelight_pwr;
} remote_ctrl_context_t;

status_t remote_ctrl_init(remote_ctrl_context_t *remote_ctrl_ptr,
                          log_context_t *log_ptr,
                          canbc_context_t *canbc_ptr,
                          TX_BYTE_POOL *stack_pool_ptr,
                          rtcan_handle_t *rtcan_s_prt,
                          const config_remote_ctrl_t *config_ptr);

status_t remote_get_bps_reading(remote_ctrl_context_t *remote_ctrl_ptr, uint16_t *result);
status_t remote_get_apps_reading(remote_ctrl_context_t *remote_ctrl_ptr, uint16_t *result);
uint8_t remote_get_r2d_reading(remote_ctrl_context_t *remote_ctrl_ptr);
uint8_t remote_get_ts_on_reading(remote_ctrl_context_t *remote_ctrl_ptr);
uint16_t remote_get_torque_reading(remote_ctrl_context_t *remote_ctrl_ptr);
uint16_t remote_get_power_reading(remote_ctrl_context_t *remote_ctrl_ptr);

void remote_ctrl_update_canbc_states(remote_ctrl_context_t *remote_ctrl_ptr);

#endif /* REMOTE_CTRL_H */
