/*
 * Stubs for the hardware-specific modules the simulation replaces:
 *   - adc_scan: values come from sim_adc1/sim_adc2
 *   - sd_card:  backed by a disk image file when VCU_SIM_SD_IMAGE is set
 *               (VCU_SIM_SD_FAIL_AFTER=N makes writes fail after N blocks)
 *   - USB device: records that it was started
 *   - task creation failure injection (rtos_utils.c is built with
 *     -DxTaskCreate=sim_xTaskCreate)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "adc_scan.h"
#include "heartbeat.h"
#include "sd_card.h"
#include "sim.h"
#include "trc.h"
#include "watchdog.h"

volatile uint16_t sim_adc1[5];
volatile uint16_t sim_adc2[1];
volatile unsigned sim_sd_writes;
volatile unsigned sim_usb_init_calls;
const char *volatile sim_fail_task_create;

status_t adc_scan_init(void) { return STATUS_OK; }
uint16_t adc_scan_get_average(const ADC_HandleTypeDef *hadc, uint8_t slot)
{
    return (hadc->Instance == ADC2) ? sim_adc2[slot] : sim_adc1[slot];
}
bool adc_scan_is_ready(const ADC_HandleTypeDef *hadc) { (void)hadc; return true; }

/* ---------------------------------------------------------------- SD card */

static FILE *sd_img;
static long sd_fail_after = -1;

static const char *sd_image_path(void)
{
    const char *p = getenv("VCU_SIM_SD_IMAGE");
    return (p && *p) ? p : NULL;
}

bool sd_card_is_inserted(void) { return sd_image_path() != NULL; }

status_t sd_card_init(void)
{
    if (!sd_image_path())
        return STATUS_ERROR;
    if (!sd_img)
        sd_img = fopen(sd_image_path(), "r+b");
    const char *f = getenv("VCU_SIM_SD_FAIL_AFTER");
    if (f && *f)
        sd_fail_after = atol(f);
    return sd_img ? STATUS_OK : STATUS_ERROR;
}

bool sd_card_is_ready(void) { return sd_img != NULL; }

status_t sd_card_get_info(uint32_t *block_count, uint32_t *block_size)
{
    if (!sd_img)
        return STATUS_ERROR;
    fseek(sd_img, 0, SEEK_END);
    *block_count = (uint32_t)(ftell(sd_img) / 512);
    *block_size = 512;
    return STATUS_OK;
}

status_t sd_card_read(uint8_t *buf, uint32_t block, uint32_t count)
{
    if (!sd_img || fseek(sd_img, (long)block * 512, SEEK_SET) != 0 ||
        fread(buf, 512, count, sd_img) != count)
        return STATUS_ERROR;
    return STATUS_OK;
}

status_t sd_card_write(const uint8_t *buf, uint32_t block, uint32_t count)
{
    if (!sd_img)
        return STATUS_ERROR;
    if (sd_fail_after >= 0 && (long)sim_sd_writes >= sd_fail_after)
        return STATUS_ERROR;
    if (fseek(sd_img, (long)block * 512, SEEK_SET) != 0 ||
        fwrite(buf, 512, count, sd_img) != count)
        return STATUS_ERROR;
    fflush(sd_img);
    sim_sd_writes += count;
    return STATUS_OK;
}

/* --------------------------------------------------------------- watchdog */

/* Model of the IWDG: records the kicks, and when the time since the last kick
 * exceeds the shortest possible real timeout (reload 500 x prescaler 32 at the
 * fastest LSI, 47 kHz = 340 ms) it records the moment the VCU would reset. */
#define SIM_WDG_MIN_TIMEOUT_MS 340U

volatile int sim_wdg_started;
volatile unsigned sim_wdg_kicks;
volatile uint32_t sim_wdg_max_gap_ms;
volatile uint32_t sim_wdg_expired_at;
static volatile TickType_t wdg_last_kick;

void watchdog_check_reset_cause(void) {}

bool watchdog_caused_last_reset(void)
{
    const char *e = getenv("VCU_SIM_WATCHDOG_RESET");
    return e && *e == '1';
}

status_t watchdog_start(void)
{
    if (!sim_wdg_started)
    {
        wdg_last_kick = xTaskGetTickCount();
        sim_wdg_started = 1;
    }
    return STATUS_OK;
}

void watchdog_kick(void)
{
    if (!sim_wdg_started)
        return;
    TickType_t now = xTaskGetTickCount();
    if (now - wdg_last_kick > sim_wdg_max_gap_ms)
        sim_wdg_max_gap_ms = now - wdg_last_kick;
    wdg_last_kick = now;
    sim_wdg_kicks++;
}

void sim_wdg_reset_gap(void)
{
    sim_wdg_max_gap_ms = 0;
}

/* checks the countdown every millisecond (started from main) */
void sim_wdg_task(void *arg)
{
    (void)arg;
    for (;;)
    {
        vTaskDelay(1);
        if (sim_wdg_started && !sim_wdg_expired_at &&
            xTaskGetTickCount() - wdg_last_kick > SIM_WDG_MIN_TIMEOUT_MS)
        {
            sim_wdg_expired_at = xTaskGetTickCount();
            printf("  [sim] watchdog expired: the VCU would reset now\n");
        }
    }
}

/* -------------------------------------------------------------------- USB */

int MX_USB_DEVICE_Init(void)
{
    sim_usb_init_calls++;
    return 0;
}

/* -------------------------------------------------------- task creation */

BaseType_t sim_xTaskCreate(TaskFunction_t fn, const char *name, configSTACK_DEPTH_TYPE depth,
                           void *arg, UBaseType_t prio, TaskHandle_t *handle)
{
    if (sim_fail_task_create && strcmp(name, sim_fail_task_create) == 0)
    {
        printf("  [sim] failing creation of task %s\n", name);
        return errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
    }
    return xTaskCreate(fn, name, depth, arg, prio, handle);
}

/* same handling as MX_FREERTOS_Init() in src/Core/Src/freertos.c */
void sim_freertos_init_failure_path(vcu_context_t *vcu_ptr)
{
    if (vcu_ptr->ctrl.thread != NULL)
    {
        vTaskSuspend(vcu_ptr->ctrl.thread);
    }
    trc_set_ts_on(GPIO_PIN_RESET);
    heartbeat_set_fast();
}

/* ------------------------------------------------------------------ hooks */

void vApplicationMallocFailedHook(void)
{
    printf("MALLOC FAILED\n");
    abort();
}
