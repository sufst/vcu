/**
 ******************************************************************************
 * @file    usb_device.h
 * @brief   USB device (mass storage class) initialisation
 ******************************************************************************
 */

#ifndef __USB_DEVICE__H__
#define __USB_DEVICE__H__

#include "usbd_def.h"

extern USBD_HandleTypeDef hUsbDeviceFS;

int MX_USB_DEVICE_Init(void);

#endif /* __USB_DEVICE__H__ */
