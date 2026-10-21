/**
 ******************************************************************************
 * @file    usbd_storage_if.c
 * @brief   USB mass storage media interface: exposes the SD card as one LUN
 *
 * @details These callbacks are called from the USB interrupt, see sd_card.c
 *          for how SD transfers are completed in that context.
 ******************************************************************************
 */

#include "usbd_storage_if.h"

#include "sd_card.h"

#define STORAGE_LUN_NBR 1U

static int8_t STORAGE_Init_FS(uint8_t lun);
static int8_t STORAGE_GetCapacity_FS(uint8_t lun, uint32_t *block_num, uint16_t *block_size);
static int8_t STORAGE_IsReady_FS(uint8_t lun);
static int8_t STORAGE_IsWriteProtected_FS(uint8_t lun);
static int8_t STORAGE_Read_FS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
static int8_t STORAGE_Write_FS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
static int8_t STORAGE_GetMaxLun_FS(void);

/* standard SCSI inquiry data (36 bytes), same identity as the USBX build */
static int8_t STORAGE_Inquirydata_FS[] = {
    0x00,                                           // direct access block device
    (int8_t)0x80,                                   // removable
    0x02,                                           // version
    0x02,                                           // response data format
    (STANDARD_INQUIRY_DATA_LEN - 5),                // additional length
    0x00, 0x00, 0x00,
    'S', 'U', 'F', 'S', 'T', ' ', ' ', ' ',         // vendor (8 bytes)
    'V', 'C', 'U', ' ', 'S', 'D', ' ', 'C',         // product (16 bytes)
    'a', 'r', 'd', ' ', ' ', ' ', ' ', ' ',
    '1', '.', '0', ' ',                             // revision (4 bytes)
};

USBD_StorageTypeDef USBD_Storage_Interface_fops_FS = {
    STORAGE_Init_FS,
    STORAGE_GetCapacity_FS,
    STORAGE_IsReady_FS,
    STORAGE_IsWriteProtected_FS,
    STORAGE_Read_FS,
    STORAGE_Write_FS,
    STORAGE_GetMaxLun_FS,
    STORAGE_Inquirydata_FS,
};

static int8_t STORAGE_Init_FS(uint8_t lun)
{
    UNUSED(lun);

    // the card is initialised before the USB device is started
    return USBD_OK;
}

static int8_t STORAGE_GetCapacity_FS(uint8_t lun, uint32_t *block_num, uint16_t *block_size)
{
    UNUSED(lun);
    uint32_t count = 0;
    uint32_t size = 0;

    if (sd_card_get_info(&count, &size) != STATUS_OK)
    {
        return USBD_FAIL;
    }

    *block_num = count;
    *block_size = (uint16_t)size;
    return USBD_OK;
}

static int8_t STORAGE_IsReady_FS(uint8_t lun)
{
    UNUSED(lun);
    return sd_card_is_ready() ? USBD_OK : USBD_FAIL;
}

static int8_t STORAGE_IsWriteProtected_FS(uint8_t lun)
{
    UNUSED(lun);
    return USBD_OK;
}

static int8_t STORAGE_Read_FS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
    UNUSED(lun);
    return (sd_card_read(buf, blk_addr, blk_len) == STATUS_OK) ? USBD_OK : USBD_FAIL;
}

static int8_t STORAGE_Write_FS(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
    UNUSED(lun);
    return (sd_card_write(buf, blk_addr, blk_len) == STATUS_OK) ? USBD_OK : USBD_FAIL;
}

static int8_t STORAGE_GetMaxLun_FS(void)
{
    return (int8_t)(STORAGE_LUN_NBR - 1U);
}
