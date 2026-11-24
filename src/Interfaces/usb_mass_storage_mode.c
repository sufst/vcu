#include "usb_mass_storage_mode.h"

#include "heartbeat.h"
#include "log.h"
#include "main.h"
#include "rtos_utils.h"
#include "sd_card.h"
#include "usb_device.h"

static void usb_msc_thread_entry(void *arg);

bool usb_mass_storage_mode_button_held(void)
{
    // USER_BUTTON has an external pullup, so is LOW if held
    return !HAL_GPIO_ReadPin(USER_BUTTON_GPIO_Port, USER_BUTTON_Pin);
}

/**
 * @brief       Starts USB mass storage mode
 *
 * @details     The SD card and USB device are initialised from a thread, as
 *              this needs the HAL tick (not available until the scheduler
 *              has started)
 */
status_t usb_msc_init(usb_msc_context_t *ctx, const config_usb_msc_t *config_ptr)
{
    return rtos_task_create(&config_ptr->thread, usb_msc_thread_entry, ctx, &ctx->thread);
}

/**
 * @brief       Initialises the SD card and USB device, then deletes itself
 *
 * @details     Once started, all USB transfers are handled from the USB
 *              interrupt. The VCU must be power cycled to exit this mode.
 */
static void usb_msc_thread_entry(void *arg)
{
    (void)arg;

    LOG_INFO("Entering USB mass storage mode\n");

    LOG_INFO("USB: SD init\n");
    if (sd_card_init() != STATUS_OK)
    {
        LOG_ERROR("USB MSC: SD card init failed\n");
        vTaskDelete(NULL);
    }

    LOG_INFO("USB: device init\n");
    if (MX_USB_DEVICE_Init() != 0)
    {
        LOG_ERROR("USB MSC: USB device init failed\n");
        vTaskDelete(NULL);
    }

    LOG_INFO("USB MSC: active, power cycle to exit\n");

    heartbeat_set_fast();

    vTaskDelete(NULL);
}
