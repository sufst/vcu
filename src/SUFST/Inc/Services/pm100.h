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

#include <can_c.h>
#include <rtcan.h>
#include <stdint.h>
#include <tx_api.h>

#include "config.h"
#include "log.h"
#include "status.h"

/*
 * error codes
 */
#define PM100_ERROR_NONE              0x00 // no errors
#define PM100_ERROR_INIT              0x01 // initialisation error
#define PM100_ERROR_BROADCAST_TIMEOUT 0x02 // no broadcasts received
#define PM100_ERROR_POST_FAULT        0x04 // power-on self-test fault
#define PM100_ERROR_RUN_FAULT         0x08 // runtime fault

#define PM100_RX_QUEUE_SIZE           10 // 10 items

/**
 * @brief   PM100 context
 */
typedef struct
{
     TX_THREAD thread;
     rtcan_handle_t* rtcan_c_ptr;
     rtcan_handle_t* rtcan_s_ptr;
     TX_QUEUE can_rx_queue;
     ULONG can_rx_queue_mem[PM100_RX_QUEUE_SIZE];
     TX_MUTEX state_mutex;
     bool broadcasts_valid;
     struct can_c_pm100_internal_states_t states;
     struct can_c_pm100_fault_codes_t faults;
     struct can_c_pm100_temperature_set_1_t temp1;
     struct can_c_pm100_temperature_set_2_t temp2;
     struct can_c_pm100_temperature_set_3_t temp3;
     uint16_t error;
     const config_pm100_t* config_ptr;
} pm100_context_t;

/*
 * public functions
 */
status_t pm100_init(pm100_context_t* pm100_ptr,
                    log_context_t* log_ptr,
                    TX_BYTE_POOL* stack_pool_ptr,
                    rtcan_handle_t* rtcan_c_ptr,
                    rtcan_handle_t* rtcan_s_ptr,
                    const config_pm100_t* config_ptr);
status_t pm100_lvs_on(pm100_context_t* pm100_ptr);
status_t pm100_lvs_off(pm100_context_t* pm100_ptr);
bool pm100_is_precharged(pm100_context_t* pm100_ptr);
int16_t pm100_motor_temp(pm100_context_t *pm100_ptr);
int16_t pm100_max_inverter_temp(pm100_context_t *pm100_ptr);
status_t pm100_disable(pm100_context_t* pm100_ptr);
status_t pm100_request_torque(pm100_context_t* pm100_ptr, uint16_t torque);

#endif
