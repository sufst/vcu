/******************************************************************************
 * @file    pm100.h
 * @author  Tim Brewis (@t-bre)
 * @author  Toby Godfrey (@_tg03, tmag1g21@soton.ac.uk)
 * @brief   PM100 service
 * @details This service is responsible for handling communications with the
 *          PM100DZ inverter over CAN. This includes initiating the pre-charge
 *          sequence, enabling the inverter, sending torque requests and
 *          monitoring the PM100 broadcast states
 *****************************************************************************/

#ifndef PM100_H
#define PM100_H

#include <can_t.h>
#include <rtcan.h>
#include <stdint.h>
#include <tx_api.h>

#include "config.h"
#include "log.h"
#include "status.h"

/*
 * error codes
 */
#define PM100_ERROR_NONE 0x00              // no errors
#define PM100_ERROR_INIT 0x01              // initialisation error
#define PM100_ERROR_BROADCAST_TIMEOUT 0x02 // no broadcasts received
#define PM100_ERROR_POST_FAULT 0x04        // power-on self-test fault
#define PM100_ERROR_RUN_FAULT 0x08         // runtime fault

#define PM100_RX_QUEUE_SIZE \
    48 // items of headroom against bursts of the 8 subscribed frame types

/**
 * @brief   PM100 context
 */
typedef struct
{
    TX_THREAD thread;
    rtcan_handle_t *rtcan_t_ptr;
    rtcan_handle_t *rtcan_s_ptr;
    rtcan_queue_t can_rx_queue;
    uint32_t can_rx_queue_mem[RTCAN_OS_QUEUE_MEM_SIZE(PM100_RX_QUEUE_SIZE, sizeof(rtcan_msg_t *)) / sizeof(uint32_t)];
    TX_MUTEX state_mutex;
    bool broadcasts_valid;
    struct can_t_pm100_internal_states_t states;
    struct can_t_pm100_fault_codes_t faults;
    struct can_t_pm100_temperature_set_1_t temp1;
    struct can_t_pm100_temperature_set_2_t temp2;
    struct can_t_pm100_temperature_set_3_t temp3;
    struct can_t_pm100_motor_position_info_t info;
    struct can_t_pm100_voltage_info_t voltage_info;
    struct can_t_pm100_current_info_t current_info;
    uint16_t error;
    const config_pm100_t *config_ptr;
    bool reverse_mode_dangerous;
} pm100_context_t;

/*
 * public functions
 */
status_t pm100_init(pm100_context_t *pm100_ptr,
                    TX_BYTE_POOL *stack_pool_ptr,
                    rtcan_handle_t *rtcan_t_ptr,
                    rtcan_handle_t *rtcan_s_ptr,
                    const config_pm100_t *config_ptr);
status_t pm100_lvs_on(pm100_context_t *pm100_ptr);
status_t pm100_lvs_off(pm100_context_t *pm100_ptr);
bool pm100_is_precharged(pm100_context_t *pm100_ptr);
int16_t pm100_motor_temp(pm100_context_t *pm100_ptr);
int16_t pm100_max_inverter_temp(pm100_context_t *pm100_ptr);
int16_t pm100_motor_speed(pm100_context_t *pm100_ptr);
float pm100_electrical_power_w(pm100_context_t *pm100_ptr);
status_t pm100_disable(pm100_context_t *pm100_ptr);
status_t pm100_request_torque(pm100_context_t *pm100_ptr, uint16_t torque);
void pm100_clear_error(pm100_context_t *pm100_ptr);

#endif
