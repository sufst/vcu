/***************************************************************************
 * @file   sd_logging.h
 * @author Martin Perreau (@maartin0)
 * @brief  SD card helpers + logging service
 ***************************************************************************/

#ifndef SD_LOGGING_H
#define SD_LOGGING_H

#include <stdbool.h>
#include <tx_api.h>

#include "app_filex.h"
#include "log.h"

#define SD_LOG_MSG_SIZE_ULONG \
    (LOG_MSG_MAX_TRANSMITION_LEN / sizeof(ULONG)) // 16
#define SD_LOG_QUEUE_DEPTH 64
#define SD_LOG_QUEUE_SIZE (SD_LOG_QUEUE_DEPTH * SD_LOG_MSG_SIZE_ULONG) // 2048

typedef struct
{
    TX_THREAD thread;
    char log_filename[32];
    TX_QUEUE msg_queue;
    ULONG msg_queue_mem[SD_LOG_QUEUE_SIZE];
    FX_MEDIA sd_media;
    FX_FILE log_file;
    UCHAR media_buffer[512] __attribute__((aligned(32)));
    const config_sd_t *config_ptr;
} sd_context_t;

status_t sd_init(sd_context_t *sd, TX_BYTE_POOL *stack_pool_ptr, const config_sd_t *config_ptr);

void write_sd_log_line(const char *line);

#endif
