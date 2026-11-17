/***************************************************************************
 * @file   usb_mass_storage_mode.h
 * @author Martin Perreau (@maartin0)
 * @brief  USB mass storage passthrough mode (exposes SD card to host PC)
 ***************************************************************************/

#ifndef USB_MASS_STORAGE_MODE_H
#define USB_MASS_STORAGE_MODE_H

#include <FreeRTOS.h>
#include <stdbool.h>
#include <task.h>

#include "config.h"
#include "status.h"

typedef struct
{
    TaskHandle_t thread;
} usb_msc_context_t;

bool usb_mass_storage_mode_button_held(void);
status_t usb_msc_init(usb_msc_context_t *ctx, const config_usb_msc_t *config_ptr);

#endif
