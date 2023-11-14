/*****************************************************************************
 * @file   heartbeat.h,
 * @author George Peppard (@inventor02, gjp1g21@soton.ac.uk)
 * @brief  Heartbeat service prototype
 ****************************************************************************/

#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include <tx_api.h>

#include "config.h"
#include "status.h"

typedef struct
{
    TX_THREAD thread;
    const config_heartbeat_t* config_ptr;
} heartbeat_context_t;

status_t heartbeat_init(heartbeat_context_t* heartbeat_h,
                        TX_BYTE_POOL* stack_pool_ptr,
                        const config_heartbeat_t* config_ptr);

#endif