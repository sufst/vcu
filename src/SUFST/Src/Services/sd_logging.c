#include "sd_logging.h"

#include <stdio.h>
#include <string.h>

#include "log.h"
#include "main.h"

static sd_context_t *global_sd_context;

static void sd_thread_entry(ULONG input);

static bool sd_is_connected()
{
    // SD_CARD_DETECT has an external pullup, so is LOW if the SD card is inserted
    return !HAL_GPIO_ReadPin(SD_CARD_DETECT_GPIO_Port, SD_CARD_DETECT_Pin);
}

static UINT create_log_file(sd_context_t *sd)
{
    char fname[16];
    sd->log_filename[0] = '\0';

    for (UINT n = 1; n <= 999; n++)
    {
        snprintf(fname, sizeof(fname), "log_%u.txt", n);
        FX_FILE test;
        UINT s = fx_file_open(&sd->sd_media, &test, fname, FX_OPEN_FOR_READ);
        if (s == FX_NOT_FOUND)
        {
            strncpy(sd->log_filename, fname, sizeof(sd->log_filename) - 1);
            break;
        }
        if (s == FX_SUCCESS)
        {
            fx_file_close(&test);
            continue;
        }
        return s;
    }

    if (sd->log_filename[0] == '\0')
        return FX_NO_MORE_SPACE;

    UINT s = fx_file_create(&sd->sd_media, sd->log_filename);
    if (s != FX_SUCCESS && s != FX_ALREADY_CREATED)
        return s;

    return fx_file_open(&sd->sd_media, &sd->log_file, sd->log_filename, FX_OPEN_FOR_WRITE);
}

status_t sd_init(sd_context_t *sd, TX_BYTE_POOL *stack_pool_ptr, const config_sd_t *config_ptr)
{
    sd->config_ptr = config_ptr;

    if (!config_ptr->enable)
    {
        LOG_INFO("SD card disabled in config\n");
        return STATUS_OK;
    }

    if (!sd_is_connected())
    {
        LOG_ERROR("SD card not detected!\n");
        return STATUS_OK;
    }

    LOG_INFO("Enabling SD card\n");

    void *stack_ptr = NULL;
    LOG_INFO("SD: allocate stack\n");
    UINT tx_status = tx_byte_allocate(stack_pool_ptr, &stack_ptr,
                                      config_ptr->thread.stack_size, TX_NO_WAIT);

    if (tx_status == TX_SUCCESS)
    {
        LOG_INFO("SD: allocate queue\n");
        tx_status = tx_queue_create(&sd->msg_queue, NULL, SD_LOG_MSG_SIZE_ULONG,
                                    sd->msg_queue_mem, sizeof(sd->msg_queue_mem));
    }

    if (tx_status == TX_SUCCESS)
    {
        LOG_INFO("SD: allocate thread\n");
        tx_status =
            tx_thread_create(&sd->thread, (CHAR *)config_ptr->thread.name, sd_thread_entry,
                             (ULONG)sd, stack_ptr, config_ptr->thread.stack_size,
                             config_ptr->thread.priority, config_ptr->thread.priority,
                             TX_NO_TIME_SLICE, TX_AUTO_START);
    }

    if (tx_status != TX_SUCCESS)
    {
        LOG_ERROR("SD init failure\n");
        return STATUS_ERROR;
    }

    // Set global context only after queue is initialised
    global_sd_context = sd;

    LOG_INFO("SD card init success\n");

    return STATUS_OK;
}

void write_sd_log_line(const char *line)
{
    if (global_sd_context == NULL)
        return;

    char buf[LOG_MSG_MAX_TRANSMITION_LEN];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    tx_queue_send(&global_sd_context->msg_queue, buf, TX_NO_WAIT);
}

static void sd_thread_entry(ULONG input)
{
    sd_context_t *sd = (sd_context_t *)input;

    UINT s = fx_media_open(&sd->sd_media, "SD", fx_stm32_sd_driver, NULL,
                           sd->media_buffer, sizeof(sd->media_buffer));

    if (s != FX_SUCCESS)
    {
        LOG_ERROR("SD: fx_media_open failed (%u)\n", s);
        return;
    }

    s = create_log_file(sd);
    if (s != FX_SUCCESS)
    {
        LOG_ERROR("SD: failed to create log file (%u)\n", s);
        fx_media_close(&sd->sd_media);
        return;
    }

    LOG_INFO("SD: logging to %s\n", sd->log_filename);

    char line[LOG_MSG_MAX_TRANSMITION_LEN];
    UINT write_status;

    while (1)
    {
        while (tx_queue_receive(&sd->msg_queue, line, TX_NO_WAIT) == TX_SUCCESS)
        {
            write_status = fx_file_write(&sd->log_file, line, strlen(line));
            if (write_status != FX_SUCCESS)
            {
                global_sd_context = NULL;
                LOG_ERROR("SD: fx_file_write failed (%u)\n", write_status);
                fx_file_close(&sd->log_file);
                fx_media_close(&sd->sd_media);
                return;
            }
        }

        fx_media_flush(&sd->sd_media);
        tx_thread_sleep(sd->config_ptr->flush_period_ticks);
    }
}
