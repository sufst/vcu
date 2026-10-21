/**
 ******************************************************************************
 * @file    usbd_desc.c
 * @brief   USB device descriptors (full speed)
 * @details VID/PID and strings are the same as the previous USBX build.
 ******************************************************************************
 */

#include "usbd_desc.h"

#include "usbd_conf.h"
#include "usbd_core.h"

#define USBD_VID                     0x0483 // STMicroelectronics
#define USBD_PID                     0x5710
#define USBD_LANGID_STRING           0x0409 // English (US)
#define USBD_MANUFACTURER_STRING     "STMicroelectronics"
#define USBD_PRODUCT_STRING          "STM32 USB Device"
#define USBD_CONFIGURATION_STRING    "MSC Config"
#define USBD_INTERFACE_STRING        "MSC Interface"

#define USB_SIZ_STRING_SERIAL        0x1AU

static uint8_t *USBD_FS_DeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *USBD_FS_LangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *USBD_FS_ManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *USBD_FS_ProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *USBD_FS_SerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *USBD_FS_ConfigStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *USBD_FS_InterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);

static void Get_SerialNum(void);
static void IntToUnicode(uint32_t value, uint8_t *pbuf, uint8_t len);

USBD_DescriptorsTypeDef FS_Desc = {
    USBD_FS_DeviceDescriptor,     USBD_FS_LangIDStrDescriptor,
    USBD_FS_ManufacturerStrDescriptor, USBD_FS_ProductStrDescriptor,
    USBD_FS_SerialStrDescriptor,  USBD_FS_ConfigStrDescriptor,
    USBD_FS_InterfaceStrDescriptor,
};

__ALIGN_BEGIN static uint8_t USBD_FS_DeviceDesc[USB_LEN_DEV_DESC] __ALIGN_END = {
    0x12,                       // bLength
    USB_DESC_TYPE_DEVICE,       // bDescriptorType
    0x00,                       // bcdUSB
    0x02,
    0x00,                       // bDeviceClass (defined by interface)
    0x00,                       // bDeviceSubClass
    0x00,                       // bDeviceProtocol
    USB_MAX_EP0_SIZE,           // bMaxPacketSize
    LOBYTE(USBD_VID),           // idVendor
    HIBYTE(USBD_VID),
    LOBYTE(USBD_PID),           // idProduct
    HIBYTE(USBD_PID),
    0x00,                       // bcdDevice rel. 2.00
    0x02,
    USBD_IDX_MFC_STR,           // index of manufacturer string
    USBD_IDX_PRODUCT_STR,       // index of product string
    USBD_IDX_SERIAL_STR,        // index of serial number string
    USBD_MAX_NUM_CONFIGURATION, // bNumConfigurations
};

__ALIGN_BEGIN static uint8_t USBD_LangIDDesc[USB_LEN_LANGID_STR_DESC] __ALIGN_END = {
    USB_LEN_LANGID_STR_DESC,
    USB_DESC_TYPE_STRING,
    LOBYTE(USBD_LANGID_STRING),
    HIBYTE(USBD_LANGID_STRING),
};

__ALIGN_BEGIN static uint8_t USBD_StrDesc[USBD_MAX_STR_DESC_SIZ] __ALIGN_END;

__ALIGN_BEGIN static uint8_t USBD_StringSerial[USB_SIZ_STRING_SERIAL] __ALIGN_END = {
    USB_SIZ_STRING_SERIAL,
    USB_DESC_TYPE_STRING,
};

static uint8_t *USBD_FS_DeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    UNUSED(speed);
    *length = sizeof(USBD_FS_DeviceDesc);
    return USBD_FS_DeviceDesc;
}

static uint8_t *USBD_FS_LangIDStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    UNUSED(speed);
    *length = sizeof(USBD_LangIDDesc);
    return USBD_LangIDDesc;
}

static uint8_t *USBD_FS_ProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    UNUSED(speed);
    USBD_GetString((uint8_t *)USBD_PRODUCT_STRING, USBD_StrDesc, length);
    return USBD_StrDesc;
}

static uint8_t *USBD_FS_ManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    UNUSED(speed);
    USBD_GetString((uint8_t *)USBD_MANUFACTURER_STRING, USBD_StrDesc, length);
    return USBD_StrDesc;
}

static uint8_t *USBD_FS_SerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    UNUSED(speed);
    *length = USB_SIZ_STRING_SERIAL;

    // serial number derived from the MCU unique ID
    Get_SerialNum();

    return USBD_StringSerial;
}

static uint8_t *USBD_FS_ConfigStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    UNUSED(speed);
    USBD_GetString((uint8_t *)USBD_CONFIGURATION_STRING, USBD_StrDesc, length);
    return USBD_StrDesc;
}

static uint8_t *USBD_FS_InterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
    UNUSED(speed);
    USBD_GetString((uint8_t *)USBD_INTERFACE_STRING, USBD_StrDesc, length);
    return USBD_StrDesc;
}

static void Get_SerialNum(void)
{
    uint32_t serial0 = HAL_GetUIDw0();
    uint32_t serial1 = HAL_GetUIDw1();
    uint32_t serial2 = HAL_GetUIDw2();

    serial0 += serial2;

    if (serial0 != 0U)
    {
        IntToUnicode(serial0, &USBD_StringSerial[2], 8);
        IntToUnicode(serial1, &USBD_StringSerial[18], 4);
    }
}

static void IntToUnicode(uint32_t value, uint8_t *pbuf, uint8_t len)
{
    for (uint8_t idx = 0; idx < len; idx++)
    {
        uint8_t nibble = (uint8_t)(value >> 28);
        pbuf[2U * idx] = (uint8_t)((nibble < 0xAU) ? (nibble + '0') : (nibble + 'A' - 10));
        pbuf[(2U * idx) + 1U] = 0;
        value <<= 4;
    }
}
