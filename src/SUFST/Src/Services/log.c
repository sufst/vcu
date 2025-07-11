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
#include <tx_api.h>
#include <usart.h>

#include "config.h"
#include "status.h"

// internal function prototype
static void log_thread_entry(ULONG thread_input);

// log level names
static const char* log_level_names[]
    = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

// Global log Context
static log_context_t* global_log_context;

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
    global_log_context = log_ptr;

    status_t status = STATUS_OK;

    // allocate stack space
    void* stack_ptr = NULL;
    UINT tx_status = tx_byte_allocate(stack_pool_ptr,
                                      &stack_ptr,
                                      config_ptr->thread.stack_size,
                                      TX_NO_WAIT);

    // create log message queue
    if (tx_status == TX_SUCCESS)
    {
        tx_status = tx_queue_create(&log_ptr->msg_queue,
                                    NULL,
                                    TX_16_ULONG,
                                    log_ptr->msg_queue_mem,
                                    LOG_MSG_QUEUE_SIZE);
    }

    // create UART mutex
    if (tx_status == TX_SUCCESS)
    {
        tx_status = tx_mutex_create(&log_ptr->uart_mutex, NULL, TX_INHERIT);
    }

    // create TX thread
    if (tx_status == TX_SUCCESS)
    {
        tx_status = tx_thread_create(&log_ptr->thread,
                                     (CHAR*) log_ptr->config_ptr->thread.name,
                                     log_thread_entry,
                                     (ULONG) log_ptr,
                                     stack_ptr,
                                     config_ptr->thread.stack_size,
                                     config_ptr->thread.priority,
                                     config_ptr->thread.priority,
                                     TX_NO_TIME_SLICE,
                                     TX_AUTO_START);
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
        return status;
    }

    return status;
}

status_t log_printf(const config_log_level_t level, const char* format, ...)
{
    // check if the message should be logged
    if (level < global_log_context->config_ptr->min_level)
        return STATUS_OK;

    // create the message
    log_msg_t msg;
    msg.level = level;

    // timestamp message
    msg.timestamp = tx_time_get();

    // format the message
    UINT last_interrupt_state = tx_interrupt_control(TX_INT_DISABLE);
    va_list args;
    va_start(args, format);
    vsnprintf(msg.msg, LOG_MSG_MAX_LEN, format, args);
    va_end(args);
    tx_interrupt_control(last_interrupt_state);

    // queue the message
    UINT tx_status = tx_queue_send(&global_log_context->msg_queue,
                                   (void*) &msg,
                                   TX_NO_WAIT);

    // check for errors
    if (tx_status != TX_SUCCESS)
    {
        return STATUS_ERROR;
    }

    return STATUS_OK;
}

void log_thread_entry(ULONG thread_input)
{
    log_context_t* log_ptr = (log_context_t*) thread_input;
    log_msg_t msg;
    UINT tx_status;

    LOG_INFO("Logging service started, min level: %s\n",
             log_level_names[log_ptr->config_ptr->min_level]);

    while (true)
    {
        // wait for a message to be queued
        tx_status = tx_queue_receive(&log_ptr->msg_queue,
                                     (void*) &msg,
                                     TX_WAIT_FOREVER);

        // if no message was recieved, try again
        // TODO: Error Handling
        if (tx_status != TX_SUCCESS)
            continue;

        // Create a buffer for the log message
        char log_msg_to_send[LOG_MSG_MAX_TRANSMITION_LEN];

        // format the log message
        UINT last_interrupt_state = tx_interrupt_control(TX_INT_DISABLE);
        snprintf(log_msg_to_send,
                 LOG_MSG_MAX_TRANSMITION_LEN,
                 "%ld [%s]: %s",
                 msg.timestamp,
                 log_level_names[msg.level],
                 msg.msg);
        tx_interrupt_control(last_interrupt_state);

        // lock the UART mutex
        tx_status = tx_mutex_get(&log_ptr->uart_mutex, TX_WAIT_FOREVER);

        if (tx_status != TX_SUCCESS)
        {
            log_ptr->error |= LOG_ERROR_MUTEX;
            continue;
        }
        else
        {
            log_ptr->error &= ~LOG_ERROR_MUTEX;
        }

        // print the message
        HAL_StatusTypeDef status
            = HAL_UART_Transmit(log_ptr->config_ptr->uart,
                                (const uint8_t*) log_msg_to_send,
                                strlen(log_msg_to_send),
                                HAL_MAX_DELAY);

        // unlock the UART mutex
        tx_status = tx_mutex_put(&log_ptr->uart_mutex);
        if (tx_status != TX_SUCCESS)
        {
            log_ptr->error |= LOG_ERROR_MUTEX;
        }
        else
        {
            log_ptr->error &= ~LOG_ERROR_MUTEX;
        }

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
