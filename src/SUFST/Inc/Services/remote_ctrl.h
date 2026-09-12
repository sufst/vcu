/*****************************************************************************
 * @file    remote_ctrl.h
 * @author  Dmytro Avdieienko (@Avdieienko, da3e22@soton.ac.uk)
 * @brief   Remote control service
 * @details Thread-safe remote control service implementation intended for use on the dyno
 ****************************************************************************/

#ifndef REMOTE_CTRL_H
#define REMOTE_CTRL_H

#include <can_s.h>
#include <can_t.h>
#include <rtcan.h>
#include <stdint.h>
#include <tx_api.h>

#include "canbc.h"
#include "config.h"
#include "log.h"
#include "status.h"

#define REMOTE_CTRL_RX_QUEUE_SIZE 2 // 2 items (8 bytes)

typedef struct
{
    TX_THREAD thread;
    TX_MUTEX sensor_mutex;
    const config_remote_ctrl_t *config_ptr;
    canbc_context_t *canbc_ptr;
    rtcan_handle_t *rtcan_s_ptr;
    rtcan_queue_t can_rx_queue;
    uint32_t can_rx_queue_mem[RTCAN_OS_QUEUE_MEM_SIZE(REMOTE_CTRL_RX_QUEUE_SIZE, sizeof(rtcan_msg_t *)) / sizeof(uint32_t)];

    struct can_s_vcu_simulation_t requests;
    bool ts_on_prev;
    bool r2d_prev;
} remote_ctrl_context_t;

status_t remote_ctrl_init(remote_ctrl_context_t *remote_ctrl_ptr,
                          canbc_context_t *canbc_ptr,
                          TX_BYTE_POOL *stack_pool_ptr,
                          rtcan_handle_t *rtcan_s_prt,
                          const config_remote_ctrl_t *config_ptr);

uint8_t remote_get_r2d_pressed(remote_ctrl_context_t *remote_ctrl_ptr);
uint8_t remote_get_ts_on_pressed(remote_ctrl_context_t *remote_ctrl_ptr);
uint16_t remote_get_torque_reading(remote_ctrl_context_t *remote_ctrl_ptr);

#endif /* REMOTE_CTRL_H */
