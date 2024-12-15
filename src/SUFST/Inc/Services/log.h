/*****************************************************************************
 * @file    log.h
 * @author  George Peppard (@inventor02, gjp1g21@soton.ac.uk)
 * @author  Daniel Hobbs (@D4nDude, dakh1g20@soton.ac.uk)
 * @brief   Logging service
 * @details Thread-safe logging implementation
 ****************************************************************************/

#ifndef LOG_H
#define LOG_H

#include <stdint.h>
#include <tx_api.h>

#include "config.h"
#include "status.h"

// error codes
#define LOG_ERROR_NONE  0x00 // no error
#define LOG_ERROR_INIT  0x01 // initialisation error
#define LOG_ERROR_UART  0x02 // UART transmit error
#define LOG_ERROR_MUTEX 0x04 // mutex error

// the max length for a log message
#define LOG_MSG_MAX_LEN 64

// the max length for a single line of log message transmission
#define LOG_MSG_MAX_TRANSMITION_LEN 128

typedef struct
{
    config_log_level_t level;
    char msg[LOG_MSG_MAX_LEN + 1];
} log_msg_t;

// number of log messages that can be queued
#define LOG_MSG_QUEUE_SIZE 128 * sizeof(log_msg_t)

/**
 * @brief logging service context
 */
typedef struct
{
    TX_THREAD thread;
    TX_MUTEX uart_mutex;
    TX_QUEUE msg_queue;
    ULONG msg_queue_mem[LOG_MSG_QUEUE_SIZE];
    const config_log_t* config_ptr;
    uint16_t error;
} log_context_t;

status_t log_init(log_context_t* log_ptr,
                  TX_BYTE_POOL* stack_pool_ptr,
                  const config_log_t* config_ptr);

status_t log_printf(const config_log_level_t level, const char* format, ...);

// Convenience macros
#define LOG_DEBUG(...) log_printf(LOG_LEVEL_DEBUG, ##__VA_ARGS__)
#define LOG_INFO(...)  log_printf(LOG_LEVEL_INFO, ##__VA_ARGS__)
#define LOG_WARN(...)  log_printf(LOG_LEVEL_WARN, ##__VA_ARGS__)
#define LOG_ERROR(...) log_printf(LOG_LEVEL_ERROR, ##__VA_ARGS__)
#define LOG_FATAL(...) log_printf(LOG_LEVEL_FATAL, ##__VA_ARGS__)

#endif
