/**
 ******************************************************************************
 * @file    usb_device.c
 * @brief   USB device (mass storage class) initialisation
 ******************************************************************************
 */

#include "usb_device.h"

#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_msc.h"
#include "usbd_storage_if.h"

USBD_HandleTypeDef hUsbDeviceFS;

/**
 * @brief   Initialises and starts the USB device as a mass storage device
 *
 * @retval  0 on success, non-zero on failure
 */
int MX_USB_DEVICE_Init(void)
{
    if (USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS) != USBD_OK)
    {
        return 1;
    }

    if (USBD_RegisterClass(&hUsbDeviceFS, &USBD_MSC) != USBD_OK)
    {
        return 1;
    }

    if (USBD_MSC_RegisterStorage(&hUsbDeviceFS, &USBD_Storage_Interface_fops_FS) != USBD_OK)
    {
        return 1;
    }

    if (USBD_Start(&hUsbDeviceFS) != USBD_OK)
    {
        return 1;
    }

    return 0;
}
