/******************************************************************************
 * @file    pm100.h
 * @author  Tim Brewis (@t-bre)
 * @brief   PM100 service
 * @details This service is responsible for handling communications with the
 *          PM100DZ inverter over CAN. This includes initiating the pre-charge
 *          sequence, enabling the inverter, sending torque requests and
 *          monitoring the PM100 broadcast states
 *****************************************************************************/

#ifndef PM100_H
#define PM100_H

#include <rtcan.h>
#include <stdint.h>
#include <tx_api.h>

#include "config.h"
#include "status.h"

/*
 * error codes
 */
#define PM100_ERROR_NONE              0x00 // no errors
#define PM100_ERROR_INIT              0x01 // initialisation error
#define PM100_ERROR_BROADCAST_TIMEOUT 0x02 // no broadcasts received

/**
 * @brief   PM100 context
 */
typedef struct
{
    TX_THREAD thread;
    const config_pm100_t* config_ptr;
} pm100_context_t;

/*
 * public functions
 */
status_t pm100_init(pm100_context_t* pm100_ptr,
                    TX_BYTE_POOL* stack_pool_ptr,
                    const config_pm100_t* config_ptr);

#endif