/*****************************************************************************
 * @file    log.c
 * @author  George Peppard (@inventor02, gjp1g21@soton.ac.uk)
 * @author  Daniel Hobbs (@D4nDude, dakh1g20@soton.ac.uk)
 * @brief   Logging service
 * @details Thread-safe logging implementation
 ****************************************************************************/

#include "log.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <usart.h>

#include "config.h"
#include "rs232.h"
#include "rtos_utils.h"
#include "sd_logging.h"
#include "status.h"

// internal function prototype
static void log_thread_entry(void *arg);

// log level names
static const char *log_level_names[] = { "DEBUG", "INFO", "WARN", "ERROR", "FATAL" };

// Global log Context
static log_context_t *global_log_context;

/**
 * @brief initialises the logging service
 *
 * @param log_ptr the logging service context
 * @param config_ptr logging service configuration
 * @return status_t outcome of initialisation
 */
status_t log_init(log_context_t *log_ptr, const config_log_t *config_ptr)
{
    log_ptr->config_ptr = config_ptr;
    log_ptr->error = LOG_ERROR_NONE;

    rs232_enable();

    status_t status = STATUS_OK;

    // create log message queue
    log_ptr->msg_queue = xQueueCreateStatic(LOG_MSG_QUEUE_LENGTH, sizeof(log_msg_t),
                                            log_ptr->msg_queue_mem, &log_ptr->msg_queue_cb);

    if (log_ptr->msg_queue == NULL)
    {
        status = STATUS_ERROR;
    }
    else
    {
        // messages can be queued as soon as the queue exists
        global_log_context = log_ptr;
    }

    // create the service thread
    if (status == STATUS_OK)
    {
        status = rtos_task_create(&config_ptr->thread, log_thread_entry, log_ptr,
                                  &log_ptr->thread);
    }

    // check for errors
    if (status != STATUS_OK)
    {
        log_ptr->error |= LOG_ERROR_INIT;
    }

    return status;
}

/**
 * @brief       Formats and queues a log message
 *
 * @details     Safe to call from both threads and interrupt handlers (with a
 *              priority no higher than configMAX_SYSCALL_INTERRUPT_PRIORITY).
 *              The message is dropped if the queue is full.
 */
status_t log_printf(const config_log_level_t level, const char *format, ...)
{
    if (global_log_context == NULL)
        return STATUS_ERROR;

    // check if the message should be logged
    if (level < global_log_context->config_ptr->min_level)
        return STATUS_OK;

    const bool in_isr = rtos_in_isr();

    // create the message
    log_msg_t msg;
    msg.level = level;

    // timestamp message
    msg.timestamp = in_isr ? xTaskGetTickCountFromISR() : xTaskGetTickCount();

    // format the message (the C library formatter is not re-entrant)
    va_list args;
    va_start(args, format);

    if (in_isr)
    {
        UBaseType_t saved_mask = taskENTER_CRITICAL_FROM_ISR();
        vsnprintf(msg.msg, LOG_MSG_MAX_LEN, format, args);
        taskEXIT_CRITICAL_FROM_ISR(saved_mask);
    }
    else
    {
        taskENTER_CRITICAL();
        vsnprintf(msg.msg, LOG_MSG_MAX_LEN, format, args);
        taskEXIT_CRITICAL();
    }

    va_end(args);

    // queue the message
    BaseType_t queued;

    if (in_isr)
    {
        BaseType_t higher_priority_task_woken = pdFALSE;
        queued = xQueueSendFromISR(global_log_context->msg_queue, &msg,
                                   &higher_priority_task_woken);
        portYIELD_FROM_ISR(higher_priority_task_woken);
    }
    else
    {
        queued = xQueueSend(global_log_context->msg_queue, &msg, 0);
    }

    return (queued == pdPASS) ? STATUS_OK : STATUS_ERROR;
}

static void log_thread_entry(void *arg)
{
    log_context_t *log_ptr = (log_context_t *)arg;
    log_msg_t msg;

    LOG_INFO("Logging service started, min level: %s\n",
             log_level_names[log_ptr->config_ptr->min_level]);

    while (true)
    {
        // wait for a message to be queued
        if (xQueueReceive(log_ptr->msg_queue, &msg, portMAX_DELAY) != pdPASS)
            continue;

        // Create a buffer for the log message
        char log_msg_to_send[LOG_MSG_MAX_TRANSMITION_LEN];

        // format the log message
        taskENTER_CRITICAL();
        snprintf(log_msg_to_send, LOG_MSG_MAX_TRANSMITION_LEN, "%lu [%s]: %s",
                 (unsigned long)msg.timestamp, log_level_names[msg.level], msg.msg);
        taskEXIT_CRITICAL();

        // print the message over UART (-> rs232)
        HAL_StatusTypeDef uart_status = HAL_OK;
        if (log_ptr->config_ptr->uart != NULL)
        {
            uart_status = HAL_UART_Transmit(log_ptr->config_ptr->uart,
                                            (const uint8_t *)log_msg_to_send,
                                            strlen(log_msg_to_send), HAL_MAX_DELAY);
        }

        // print the message over USART (-> debug header)
        HAL_StatusTypeDef usart_status = HAL_OK;
        if (log_ptr->config_ptr->usart != NULL)
        {
            usart_status = HAL_USART_Transmit(log_ptr->config_ptr->usart,
                                              (const uint8_t *)log_msg_to_send,
                                              strlen(log_msg_to_send), HAL_MAX_DELAY);
        }

        // if level >= sd_min_level write to SD card
        if (msg.level >= log_ptr->config_ptr->min_sd_log_level)
            write_sd_log_line(log_msg_to_send);

        if (uart_status != HAL_OK)
        {
            log_ptr->error |= LOG_ERROR_UART;
        }
        else
        {
            log_ptr->error &= ~LOG_ERROR_UART;
        }

        if (usart_status != HAL_OK)
        {
            log_ptr->error |= LOG_ERROR_USART;
        }
        else
        {
            log_ptr->error &= ~LOG_ERROR_USART;
        }
    }
}
