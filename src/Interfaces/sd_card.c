/***************************************************************************
 * @file   sd_card.c
 * @brief  SD card block device (SDMMC1 + DMA)
 *
 * @details Transfers use DMA. When called from a task, the caller blocks on
 *          a semaphore given by the SD transfer complete interrupt. When
 *          called from an interrupt handler (the USB mass storage class
 *          handles SCSI commands inside the USB interrupt), completion is
 *          polled instead; the SDMMC and SD DMA interrupts have a higher
 *          priority than the USB interrupt so they still run.
 *
 *          D-cache is not enabled in this project, so no cache maintenance
 *          is needed around the DMA transfers.
 ***************************************************************************/

#include "sd_card.h"

#include <FreeRTOS.h>
#include <semphr.h>
#include <string.h>
#include <task.h>

#include "main.h"
#include "rtos_utils.h"
#include "sdmmc.h"

// timeout for a transfer to complete, and for the card to finish programming
#define SD_CARD_TIMEOUT_MS 2000U

/*
 * DMA needs word-aligned buffers, so unaligned transfers go through this
 * buffer one block at a time. It is placed in .dma_bss so it is never in
 * DTCM (see STM32F746ZGTx_FLASH.ld).
 */
static uint32_t scratch[SD_CARD_BLOCK_SIZE / sizeof(uint32_t)] __attribute__((section(".dma_bss")));

static SemaphoreHandle_t xfer_sem;
static StaticSemaphore_t xfer_sem_cb;
static volatile bool xfer_done;
static volatile bool xfer_error;
static bool initialised;

/**
 * @brief   Returns true if an SD card is inserted
 */
bool sd_card_is_inserted(void)
{
    // SD_CARD_DETECT has an external pullup, so is LOW if the SD card is inserted
    return HAL_GPIO_ReadPin(SD_CARD_DETECT_GPIO_Port, SD_CARD_DETECT_Pin) == GPIO_PIN_RESET;
}

/**
 * @brief   Initialises the SDMMC peripheral and the card
 *
 * @details Same configuration as MX_SDMMC1_SD_Init(), but returns an error
 *          rather than calling Error_Handler(), so a faulty card cannot halt
 *          the VCU. Safe to call more than once.
 */
status_t sd_card_init(void)
{
    if (initialised)
    {
        return STATUS_OK;
    }

    if (xfer_sem == NULL)
    {
        xfer_sem = xSemaphoreCreateBinaryStatic(&xfer_sem_cb);
    }

    hsd1.Instance = SDMMC1;
    hsd1.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
    hsd1.Init.ClockBypass = SDMMC_CLOCK_BYPASS_DISABLE;
    hsd1.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_ENABLE;
    hsd1.Init.BusWide = SDMMC_BUS_WIDE_1B;
    hsd1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
    hsd1.Init.ClockDiv = 0;

    if (HAL_SD_Init(&hsd1) != HAL_OK)
    {
        return STATUS_ERROR;
    }

    if (HAL_SD_ConfigWideBusOperation(&hsd1, SDMMC_BUS_WIDE_4B) != HAL_OK)
    {
        return STATUS_ERROR;
    }

    initialised = true;
    return STATUS_OK;
}

/**
 * @brief   Returns true if the card is initialised and ready for a transfer
 */
bool sd_card_is_ready(void)
{
    return initialised && (HAL_SD_GetCardState(&hsd1) == HAL_SD_CARD_TRANSFER);
}

/**
 * @brief       Gets the card capacity
 *
 * @param[out]  block_count     Number of logical blocks
 * @param[out]  block_size      Logical block size in bytes
 */
status_t sd_card_get_info(uint32_t *block_count, uint32_t *block_size)
{
    HAL_SD_CardInfoTypeDef info;

    if (!initialised || (HAL_SD_GetCardInfo(&hsd1, &info) != HAL_OK))
    {
        return STATUS_ERROR;
    }

    *block_count = info.LogBlockNbr;
    *block_size = info.LogBlockSize;
    return STATUS_OK;
}

/**
 * @brief   Busy-waits for roughly the given number of milliseconds
 *
 * @details Used in interrupt context, where neither the RTOS nor the HAL
 *          tick (whose interrupt has the same priority as USB) can be used
 */
static void spin_delay_ms(uint32_t ms)
{
    // each iteration is at least a few cycles, so this errs on the long side
    volatile uint32_t count = ms * (SystemCoreClock / 4000U);

    while (count > 0U)
    {
        count--;
    }
}

/**
 * @brief   Waits for a DMA transfer to finish, then for the card to return to
 *          the transfer state (i.e. finish programming after a write)
 */
static status_t wait_for_completion(void)
{
    const bool in_isr = rtos_in_isr();
    bool done = false;

    if (in_isr)
    {
        for (uint32_t ms = 0; (ms < SD_CARD_TIMEOUT_MS) && !xfer_done; ms++)
        {
            spin_delay_ms(1);
        }
        done = xfer_done;
    }
    else
    {
        done = (xSemaphoreTake(xfer_sem, pdMS_TO_TICKS(SD_CARD_TIMEOUT_MS)) == pdPASS) && xfer_done;
    }

    if (!done || xfer_error)
    {
        (void)HAL_SD_Abort(&hsd1);
        return STATUS_ERROR;
    }

    for (uint32_t ms = 0; ms < SD_CARD_TIMEOUT_MS; ms++)
    {
        if (HAL_SD_GetCardState(&hsd1) == HAL_SD_CARD_TRANSFER)
        {
            return STATUS_OK;
        }

        if (in_isr)
        {
            spin_delay_ms(1);
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }

    return STATUS_ERROR;
}

static void start_transfer(void)
{
    xfer_done = false;
    xfer_error = false;

    if (!rtos_in_isr())
    {
        // clear any stale completion
        (void)xSemaphoreTake(xfer_sem, 0);
    }
}

static status_t read_aligned(uint8_t *buf, uint32_t block, uint32_t count)
{
    start_transfer();

    if (HAL_SD_ReadBlocks_DMA(&hsd1, buf, block, count) != HAL_OK)
    {
        return STATUS_ERROR;
    }

    return wait_for_completion();
}

static status_t write_aligned(const uint8_t *buf, uint32_t block, uint32_t count)
{
    start_transfer();

    if (HAL_SD_WriteBlocks_DMA(&hsd1, (uint8_t *)buf, block, count) != HAL_OK)
    {
        return STATUS_ERROR;
    }

    return wait_for_completion();
}

/**
 * @brief       Reads blocks from the card
 *
 * @param[out]  buf     Destination (any alignment)
 * @param[in]   block   First block number
 * @param[in]   count   Number of blocks
 */
status_t sd_card_read(uint8_t *buf, uint32_t block, uint32_t count)
{
    if (!initialised)
    {
        return STATUS_ERROR;
    }

    if (((uint32_t)buf % sizeof(uint32_t)) == 0U)
    {
        return read_aligned(buf, block, count);
    }

    for (uint32_t i = 0; i < count; i++)
    {
        if (read_aligned((uint8_t *)scratch, block + i, 1) != STATUS_OK)
        {
            return STATUS_ERROR;
        }

        memcpy(&buf[i * SD_CARD_BLOCK_SIZE], scratch, SD_CARD_BLOCK_SIZE);
    }

    return STATUS_OK;
}

/**
 * @brief       Writes blocks to the card
 *
 * @param[in]   buf     Source (any alignment)
 * @param[in]   block   First block number
 * @param[in]   count   Number of blocks
 */
status_t sd_card_write(const uint8_t *buf, uint32_t block, uint32_t count)
{
    if (!initialised)
    {
        return STATUS_ERROR;
    }

    if (((uint32_t)buf % sizeof(uint32_t)) == 0U)
    {
        return write_aligned(buf, block, count);
    }

    for (uint32_t i = 0; i < count; i++)
    {
        memcpy(scratch, &buf[i * SD_CARD_BLOCK_SIZE], SD_CARD_BLOCK_SIZE);

        if (write_aligned((const uint8_t *)scratch, block + i, 1) != STATUS_OK)
        {
            return STATUS_ERROR;
        }
    }

    return STATUS_OK;
}

/*
 * HAL callbacks (interrupt context)
 */
static void notify_transfer_end(bool error)
{
    xfer_error = error;
    xfer_done = true;

    if (xfer_sem != NULL)
    {
        BaseType_t higher_priority_task_woken = pdFALSE;
        (void)xSemaphoreGiveFromISR(xfer_sem, &higher_priority_task_woken);
        portYIELD_FROM_ISR(higher_priority_task_woken);
    }
}

void HAL_SD_RxCpltCallback(SD_HandleTypeDef *hsd)
{
    UNUSED(hsd);
    notify_transfer_end(false);
}

void HAL_SD_TxCpltCallback(SD_HandleTypeDef *hsd)
{
    UNUSED(hsd);
    notify_transfer_end(false);
}

void HAL_SD_ErrorCallback(SD_HandleTypeDef *hsd)
{
    UNUSED(hsd);
    notify_transfer_end(true);
}
