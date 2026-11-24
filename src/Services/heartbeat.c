/*****************************************************************************
 * @file   heartbeat.c,
 * @author George Peppard (@inventor02, gjp1g21@soton.ac.uk)
 * @brief  Heartbeat service implementation (glorified LED blinker)
 ****************************************************************************/

#include "heartbeat.h"

#include "io.h"
#include "rtos_utils.h"

static heartbeat_context_t *global_heartbeat_context;

static void heartbeat_thread_entry(void *arg);

/**
 * @brief       Initialise heartbeat service
 *
 * @param[in]   heartbeat_h     Heartbeat handle
 * @param[in]   config_ptr      Configuration
 */
status_t heartbeat_init(heartbeat_context_t *heartbeat_h, const config_heartbeat_t *config_ptr)
{
    heartbeat_h->config_ptr = config_ptr;
    heartbeat_h->fast_mode = false;

    // create service thread
    if (rtos_task_create(&config_ptr->thread, heartbeat_thread_entry, heartbeat_h,
                         &heartbeat_h->thread) != STATUS_OK)
        return STATUS_ERROR;

    global_heartbeat_context = heartbeat_h;
    return STATUS_OK;
}

void heartbeat_set_fast(void)
{
    if (global_heartbeat_context != NULL)
        global_heartbeat_context->fast_mode = true;
}

/**
 * @brief Heartbeat service thread
 * @details Flashes an LED at a defined period
 *
 * @param arg heartbeat handle
 */
static void heartbeat_thread_entry(void *arg)
{
    heartbeat_context_t *heartbeat_h = (heartbeat_context_t *)arg;

    // initialise LED
    VCU_Output_Low(ROB_LED_GPIO_Port, ROB_LED_Pin);

    // RAM report (see rtos_log_resources()): 10 s and 60 s after power-up,
    // then every 10 minutes
    TickType_t next_report = xTaskGetTickCount() + 10U * configTICK_RATE_HZ;
    bool first_report = true;

    // main loop
    while (1)
    {
        if ((TickType_t)(xTaskGetTickCount() - next_report) < (TickType_t)(configTICK_RATE_HZ * 60U))
        {
            rtos_log_resources();
            next_report += first_report ? 50U * configTICK_RATE_HZ : 600U * configTICK_RATE_HZ;
            first_report = false;
        }

        VCU_Output_Toggle(ROB_LED_GPIO_Port, ROB_LED_Pin);

        TickType_t period = heartbeat_h->fast_mode ?
            heartbeat_h->config_ptr->fast_blink_period_ticks :
            heartbeat_h->config_ptr->blink_period_ticks;
        vTaskDelay(period);
    }
}