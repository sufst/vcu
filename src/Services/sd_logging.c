#include "sd_logging.h"

#include <stdio.h>
#include <string.h>

#include "ff.h"
#include "log.h"
#include "main.h"
#include "rtos_utils.h"
#include "sd_card.h"

static sd_context_t *global_sd_context;

/*
 * FatFs objects. These contain the sector buffers that the SD card DMA reads
 * into and writes from, so they are placed in .dma_bss (outside DTCM).
 */
static FATFS sd_fs __attribute__((section(".dma_bss"), aligned(4)));
static FIL log_file __attribute__((section(".dma_bss"), aligned(4)));

static void sd_thread_entry(void *arg);

/**
 * @brief   Creates the next unused log_N.txt file and opens it for writing
 */
static FRESULT create_log_file(sd_context_t *sd)
{
    char fname[16];
    sd->log_filename[0] = '\0';

    for (unsigned int n = 1; n <= 999; n++)
    {
        snprintf(fname, sizeof(fname), "log_%u.txt", n);
        FRESULT s = f_stat(fname, NULL);
        if (s == FR_NO_FILE)
        {
            strncpy(sd->log_filename, fname, sizeof(sd->log_filename) - 1);
            break;
        }
        if (s == FR_OK)
        {
            continue;
        }
        return s;
    }

    if (sd->log_filename[0] == '\0')
        return FR_DENIED;

    return f_open(&log_file, sd->log_filename, FA_WRITE | FA_CREATE_NEW);
}

status_t sd_init(sd_context_t *sd, const config_sd_t *config_ptr)
{
    sd->config_ptr = config_ptr;

    if (!config_ptr->enable)
    {
        LOG_INFO("SD card disabled in config\n");
        return STATUS_OK;
    }

    if (!sd_card_is_inserted())
    {
        LOG_ERROR("SD card not detected!\n");
        return STATUS_OK;
    }

    LOG_INFO("Enabling SD card\n");

    status_t status = STATUS_OK;

    sd->msg_queue = xQueueCreateStatic(SD_LOG_QUEUE_DEPTH, SD_LOG_MSG_SIZE,
                                       sd->msg_queue_mem, &sd->msg_queue_cb);

    if (sd->msg_queue == NULL)
    {
        status = STATUS_ERROR;
    }

    if (status == STATUS_OK)
    {
        status = rtos_task_create(&config_ptr->thread, sd_thread_entry, sd, &sd->thread);
    }

    if (status != STATUS_OK)
    {
        LOG_ERROR("SD init failure\n");
        return STATUS_ERROR;
    }

    // Set global context only after queue is initialised
    global_sd_context = sd;

    LOG_INFO("SD card init success\n");

    return STATUS_OK;
}

/**
 * @brief   Queues a line to be written to the SD card log (non-blocking)
 */
void write_sd_log_line(const char *line)
{
    sd_context_t *sd = global_sd_context;

    if (sd == NULL)
        return;

    char buf[SD_LOG_MSG_SIZE];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    (void)xQueueSend(sd->msg_queue, buf, 0);
}

/**
 * @brief   Stops SD logging and deletes the calling (SD) task
 */
static void sd_stop(void)
{
    global_sd_context = NULL;
    vTaskDelete(NULL);
}

static void sd_thread_entry(void *arg)
{
    sd_context_t *sd = (sd_context_t *)arg;

    FRESULT s = f_mount(&sd_fs, "", 1);

    if (s != FR_OK)
    {
        LOG_ERROR("SD: f_mount failed (%u)\n", s);
        sd_stop();
    }

    s = create_log_file(sd);
    if (s != FR_OK)
    {
        LOG_ERROR("SD: failed to create log file (%u)\n", s);
        (void)f_unmount("");
        sd_stop();
    }

    LOG_INFO("SD: logging to %s\n", sd->log_filename);

    char line[SD_LOG_MSG_SIZE];
    const TickType_t flush_period = sd->config_ptr->flush_period_ticks;
    TickType_t last_sync = xTaskGetTickCount();

    // write lines as soon as they arrive (so the queue doesn't overflow and
    // drop messages), and sync the file to the card every flush period
    while (1)
    {
        const TickType_t since_sync = xTaskGetTickCount() - last_sync;
        const TickType_t wait = (since_sync < flush_period) ? (flush_period - since_sync) : 0;

        if (xQueueReceive(sd->msg_queue, line, wait) == pdPASS)
        {
            UINT written = 0;
            const UINT len = strlen(line);
            s = f_write(&log_file, line, len, &written);

            if (s != FR_OK || written != len)
            {
                global_sd_context = NULL;
                LOG_ERROR("SD: f_write failed (%u)\n", s);
                (void)f_close(&log_file);
                (void)f_unmount("");
                sd_stop();
            }
        }

        if ((xTaskGetTickCount() - last_sync) >= flush_period)
        {
            (void)f_sync(&log_file);
            last_sync = xTaskGetTickCount();
        }
    }
}
