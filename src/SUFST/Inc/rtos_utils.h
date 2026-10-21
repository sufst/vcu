/***************************************************************************
 * @file    rtos_utils.h
 * @brief   Small helpers shared by the services for using FreeRTOS
 ***************************************************************************/

#ifndef RTOS_UTILS_H
#define RTOS_UTILS_H

#include <FreeRTOS.h>
#include <stdbool.h>
#include <task.h>

#include "config.h"
#include "status.h"

status_t rtos_task_create(const config_thread_t *config_ptr,
                          TaskFunction_t entry,
                          void *arg,
                          TaskHandle_t *handle_ptr);

/**
 * @brief   Returns true if called from an interrupt handler
 */
bool rtos_every(TickType_t *last_tick, TickType_t period);
void rtos_log_resources(void);

static inline bool rtos_in_isr(void)
{
    return xPortIsInsideInterrupt() == pdTRUE;
}

#endif
