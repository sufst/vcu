/***************************************************************************
 * @file   sd_logging.h
 * @author Martin Perreau (@maartin0)
 * @brief  SD card helpers + logging service
 ***************************************************************************/

#ifndef SD_LOGGING_H
#define SD_LOGGING_H

#include <FreeRTOS.h>
#include <queue.h>
#include <stdbool.h>
#include <task.h>

#include "config.h"
#include "log.h"
#include "status.h"

#define SD_LOG_MSG_SIZE LOG_MSG_MAX_TRANSMITION_LEN
#define SD_LOG_QUEUE_DEPTH 64

typedef struct
{
    TaskHandle_t thread;
    char log_filename[32];
    QueueHandle_t msg_queue;
    StaticQueue_t msg_queue_cb;
    uint8_t msg_queue_mem[SD_LOG_QUEUE_DEPTH * SD_LOG_MSG_SIZE];
    const config_sd_t *config_ptr;
} sd_context_t;

status_t sd_init(sd_context_t *sd, const config_sd_t *config_ptr);

void write_sd_log_line(const char *line);

#endif
