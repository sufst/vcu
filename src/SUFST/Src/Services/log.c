/*****************************************************************************
 * @file    log.c
 * @author  George Peppard (@inventor02, gjp1g21@soton.ac.uk)
 * @brief   Logging service
 * @details Thread-safe logging implementation
 ****************************************************************************/

#include "log.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "status.h"

// internal function prototype
static void log_thread_entry(ULONG thread_input);

// log level names
static const char* log_level_names[] = {"DEBUG", "INFO", "WARN", "ERROR"};

/**
 * @brief initialises the logging service
 *
 * @param log_ptr the logging service context
 * @param stack_pool_ptr memory pool for thread stack
 * @param config_ptr logging service configuration
 * @return status_t outcome of initialisation
 */
status_t log_init(log_context_t* log_ptr,
                  TX_BYTE_POOL* stack_pool_ptr,
                  const config_log_t* config_ptr)
{
    log_ptr->config_ptr = config_ptr;
    log_ptr->error = LOG_ERROR_NONE;

    status_t status = STATUS_OK;

    // allocate stack space
    void* stack_ptr = NULL;
    UINT tx_status = tx_byte_allocate(stack_pool_ptr,
                                      &stack_ptr,
                                      config_ptr->thread.stack_size,
                                      TX_NO_WAIT);

    // create TX thread
    if (tx_status == TX_SUCCESS)
    {
        tx_status = tx_thread_create(&log_ptr->thread,
                                     log_ptr->config_ptr->thread.name,
                                     log_thread_entry,
                                     (ULONG) log_ptr,
                                     stack_ptr,
                                     config_ptr->thread.stack_size,
                                     config_ptr->thread.priority,
                                     config_ptr->thread.priority,
                                     TX_NO_TIME_SLICE,
                                     TX_AUTO_START);
    }

    // create log message queue
    if (tx_status == TX_SUCCESS)
    {
        tx_status = tx_queue_create(&log_ptr->msg_queue,
                                    NULL,
                                    sizeof(log_msg_t),
                                    log_ptr->msg_queue_mem,
                                    sizeof(log_ptr->msg_queue_mem));
    }

    // create UART mutex
    if (tx_status == TX_SUCCESS)
    {
        tx_status = tx_mutex_create(&log_ptr->uart_mutex, NULL, TX_INHERIT);
    }

    // check for errors
    if (tx_status != TX_SUCCESS)
    {
        status = STATUS_ERROR;
        log_ptr->error |= LOG_ERROR_INIT;
    }

    // if status is not OK, terminate the thread
    if (status != STATUS_OK)
    {
        tx_thread_terminate(&log_ptr->thread);
    }

    LOG_INFO(log_ptr,
             "Logging service initialised (min level: %s)",
             log_level_names[log_ptr->config_ptr->min_level]);

    return status;
}

status_t log_printf(log_context_t* log_ptr,
                    const config_log_level_t level,
                    const char* format,
                    ...)
{
    // check if the message should be logged
    if (level >= log_ptr->config_ptr->min_level)
    {
        // create the message
        log_msg_t msg;
        msg.level = level;

        // format the message
        va_list args;
        va_start(args, format);
        vsnprintf(msg.msg, LOG_MSG_MAX_LEN, format, args);
        va_end(args);

        // queue the message
        UINT tx_status = tx_queue_send(&log_ptr->msg_queue, &msg, TX_NO_WAIT);

        // check for errors
        if (tx_status != TX_SUCCESS)
        {
            return STATUS_ERROR;
        }
    }

    return STATUS_OK;
}

void log_thread_entry(ULONG thread_input)
{
    log_context_t* log_ptr = (log_context_t*) thread_input;

    while (true)
    {
        // wait for a message to be queued
        log_msg_t msg;
        UINT tx_status
            = tx_queue_receive(&log_ptr->msg_queue, &msg, TX_WAIT_FOREVER);

        // if a message was received, print it
        if (tx_status == TX_SUCCESS)
        {

            // ensures the string is null-terminated
            char msg_restrict[LOG_MSG_MAX_LEN + 1];
            strncpy(msg.msg, msg_restrict, LOG_MSG_MAX_LEN);
            msg_restrict[LOG_MSG_MAX_LEN] = NULL;

            // lock the UART mutex
            tx_mutex_get(&log_ptr->uart_mutex, TX_WAIT_FOREVER);

            // print the message
            HAL_StatusTypeDef status
                = HAL_UART_Transmit(log_ptr->config_ptr->uart,
                                    (uint8_t*) msg_restrict,
                                    strlen(msg_restrict),
                                    HAL_MAX_DELAY);

            // unlock the UART mutex
            tx_mutex_put(&log_ptr->uart_mutex);

            if (status != HAL_OK)
            {
                log_ptr->error |= LOG_ERROR_UART;
            }
            else
            {
                log_ptr->error &= ~LOG_ERROR_UART;
            }
        }
    }
}
