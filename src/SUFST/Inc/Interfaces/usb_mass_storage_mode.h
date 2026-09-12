/***************************************************************************
 * @file   usb_mass_storage_mode.h
 * @author Martin Perreau (@maartin0)
 * @brief  USB mass storage passthrough mode (exposes SD card to host PC)
 ***************************************************************************/

#ifndef USB_MASS_STORAGE_MODE_H
#define USB_MASS_STORAGE_MODE_H

#include <stdbool.h>

#include "config.h"
#include "status.h"
#include "tx_api.h"

typedef struct
{
    TX_THREAD thread;
    TX_BYTE_POOL *pool;
} usb_msc_context_t;

bool usb_mass_storage_mode_button_held(void);
status_t usb_msc_init(usb_msc_context_t *ctx, TX_BYTE_POOL *pool, const config_usb_msc_t *config_ptr);
status_t enter_usb_mass_storage_mode(TX_BYTE_POOL *pool);

#endif
