/***************************************************************************
 * @file    rtos_utils.c
 * @brief   Small helpers shared by the services for using FreeRTOS
 ***************************************************************************/

#include "rtos_utils.h"

#include "log.h"

/**
 * @brief       Creates (and starts) a task from a thread configuration
 *
 * @details     The task stack is allocated from the FreeRTOS heap. If the
 *              scheduler has not been started yet, the task will start
 *              running once it is.
 *
 * @param[in]   config_ptr  Thread configuration (stack size in bytes)
 * @param[in]   entry       Task entry function
 * @param[in]   arg         Argument passed to the entry function
 * @param[out]  handle_ptr  Created task handle
 */
status_t rtos_task_create(const config_thread_t *config_ptr,
                          TaskFunction_t entry,
                          void *arg,
                          TaskHandle_t *handle_ptr)
{
    const configSTACK_DEPTH_TYPE stack_depth =
        (configSTACK_DEPTH_TYPE)(config_ptr->stack_size / sizeof(StackType_t));

    BaseType_t rc = xTaskCreate(entry, config_ptr->name, stack_depth, arg,
                                (UBaseType_t)config_ptr->priority, handle_ptr);

    return (rc == pdPASS) ? STATUS_OK : STATUS_ERROR;
}

/**
 * @brief       Rate limiter, e.g. for errors that could be logged every cycle
 *
 * @details     Returns true the first time, then at most once per period.
 *              Must only be used from task context.
 *
 * @param[in,out]   last_tick   Tick of the last time true was returned
 *                              (initialise to 0)
 * @param[in]       period      Minimum ticks between two true results
 */
bool rtos_every(TickType_t *last_tick, TickType_t period)
{
    const TickType_t now = xTaskGetTickCount();

    if ((*last_tick == 0U) || ((now - *last_tick) >= period))
    {
        *last_tick = (now == 0U) ? 1U : now; // 0 means 'never'
        return true;
    }

    return false;
}

/**
 * @brief       Logs how much RAM is left: FreeRTOS heap (now and lowest ever)
 *              and the unused stack of every task (lowest ever)
 *
 * @details     Used to confirm on the real VCU that the heap and the task
 *              stack sizes in config.c have enough margin. Logs one line
 *              per task at INFO level.
 */
void rtos_log_resources(void)
{
    // static: too big for the stack of the (low priority) calling task
    static TaskStatus_t status[24];

    const UBaseType_t n = uxTaskGetSystemState(status, sizeof(status) / sizeof(status[0]), NULL);

    LOG_INFO("RAM: heap free %u B (lowest %u B), %u tasks\n", (unsigned)xPortGetFreeHeapSize(),
             (unsigned)xPortGetMinimumEverFreeHeapSize(), (unsigned)uxTaskGetNumberOfTasks());

    for (UBaseType_t i = 0; i < n; i++)
    {
        LOG_INFO("RAM: %s stack unused %u B\n", status[i].pcTaskName,
                 (unsigned)(status[i].usStackHighWaterMark * sizeof(StackType_t)));
    }
}
