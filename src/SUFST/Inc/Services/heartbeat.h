/*****************************************************************************
 * @file   heartbeat.h,
 * @author George Peppard (@inventor02, gjp1g21@soton.ac.uk)
 * @brief  Heartbeat service prototype
 ****************************************************************************/

#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include <FreeRTOS.h>
#include <stdbool.h>
#include <task.h>

#include "config.h"
#include "status.h"

typedef struct
{
    TaskHandle_t thread;
    const config_heartbeat_t *config_ptr;
    volatile bool fast_mode;
} heartbeat_context_t;

status_t heartbeat_init(heartbeat_context_t *heartbeat_h,
                        const config_heartbeat_t *config_ptr);

void heartbeat_set_fast(void);

#endif