/***************************************************************************
 * @file   fans.h
 * @author Martin Perreau (@maartin0)
 * @brief  Fan control service
 ***************************************************************************/

#ifndef FANS_H
#define FANS_H

#include <rtcan.h>
#include <stdint.h>
#include <tx_api.h>

#include "config.h"
#include "status.h"

#define FANS_RX_QUEUE_SIZE 3

typedef struct
{
    TX_THREAD thread;
    TX_SEMAPHORE sample_semaphore;
    TX_TIMER sample_timer;
    rtcan_handle_t *rtcan_s_h;
    rtcan_queue_t can_rx_queue;
    uint32_t can_rx_queue_mem[RTCAN_OS_QUEUE_MEM_SIZE(FANS_RX_QUEUE_SIZE, sizeof(rtcan_msg_t *)) / sizeof(uint32_t)];
    bool fan_switch_status; // driver override switch (from CAN)
    bool fan_thermal_pwr;   // thermal output (motor/inverter temp)
    const config_fans_t *config_ptr;
} fans_context_t;

status_t fans_init(fans_context_t *fans_h,
                   rtcan_handle_t *rtcan_s_h,
                   TX_BYTE_POOL *stack_pool_ptr,
                   const config_fans_t *config_ptr);

void fans_update_thermal(fans_context_t *fans_h, int16_t max_temp);
bool fans_output_pwr(const fans_context_t *fans_h);

#endif
