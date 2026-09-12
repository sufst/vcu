#include "usb_mass_storage_mode.h"

#include <string.h>

#include "heartbeat.h"

#include "log.h"
#include "main.h"
#include "sdmmc.h"
#include "usb_otg.h"
#include "ux_api.h"
#include "ux_dcd_stm32.h"
#include "ux_device_class_storage.h"
#include "ux_device_descriptors.h"
#include "ux_system.h"

bool usb_mass_storage_mode_button_held(void)
{
    // USER_BUTTON has an external pullup, so is LOW if held
    return !HAL_GPIO_ReadPin(USER_BUTTON_GPIO_Port, USER_BUTTON_Pin);
}

static UINT msc_media_status(VOID *storage, ULONG lun, ULONG media_id, ULONG *media_status)
{
    (void)storage;
    (void)lun;
    (void)media_id;
    *media_status = 0;
    if (HAL_SD_GetState(&hsd1) != HAL_SD_STATE_READY)
        return UX_ERROR;
    return UX_SUCCESS;
}

static UINT
msc_media_read(VOID *storage, ULONG lun, UCHAR *data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status)
{
    (void)storage;
    (void)lun;
    (void)media_status;
    if ((uint32_t)data_pointer % 4 != 0)
    {
        LOG_ERROR("msc_read: buf %p unaligned\n", data_pointer);
        return UX_ERROR;
    }
    HAL_StatusTypeDef dma_status =
        HAL_SD_ReadBlocks_DMA(&hsd1, data_pointer, (uint32_t)lba, (uint32_t)number_blocks);
    if (dma_status != HAL_OK)
    {
        LOG_ERROR("msc_read: DMA failed lba=%lu nblk=%lu halstate=%d "
                  "err=0x%lx\n",
                  lba, number_blocks, (int)HAL_SD_GetState(&hsd1),
                  HAL_SD_GetError(&hsd1));
        return UX_ERROR;
    }
    while (HAL_SD_GetState(&hsd1) != HAL_SD_STATE_READY)
        tx_thread_sleep(1);
    return UX_SUCCESS;
}

static UINT
msc_media_write(VOID *storage, ULONG lun, UCHAR *data_pointer, ULONG number_blocks, ULONG lba, ULONG *media_status)
{
    (void)storage;
    (void)lun;
    (void)media_status;
    if (HAL_SD_WriteBlocks_DMA(&hsd1, data_pointer, (uint32_t)lba,
                               (uint32_t)number_blocks) != HAL_OK)
        return UX_ERROR;
    // Wait for DMA completion first (no SDMMC command) before issuing CMD13
    while (HAL_SD_GetState(&hsd1) != HAL_SD_STATE_READY)
        tx_thread_sleep(1);
    // Now safe to poll card programming state via CMD13
    while (HAL_SD_GetCardState(&hsd1) != HAL_SD_CARD_TRANSFER)
        tx_thread_sleep(1);
    return UX_SUCCESS;
}

#define UX_DEVICE_POOL_SIZE (64 * 1024)

static void usb_msc_thread_entry(ULONG input)
{
    usb_msc_context_t *ctx = (usb_msc_context_t *)input;
    LOG_INFO("Entering USB mass storage mode\n");
    enter_usb_mass_storage_mode(ctx->pool);
}

status_t usb_msc_init(usb_msc_context_t *ctx, TX_BYTE_POOL *pool, const config_usb_msc_t *config_ptr)
{
    ctx->pool = pool;
    void *stack_ptr = NULL;
    if (tx_byte_allocate(pool, &stack_ptr, config_ptr->thread.stack_size, TX_NO_WAIT) != TX_SUCCESS)
        return STATUS_ERROR;
    if (tx_thread_create(&ctx->thread, (CHAR *)config_ptr->thread.name, usb_msc_thread_entry,
                         (ULONG)ctx, stack_ptr, config_ptr->thread.stack_size,
                         config_ptr->thread.priority, config_ptr->thread.priority,
                         TX_NO_TIME_SLICE, TX_AUTO_START) != TX_SUCCESS)
        return STATUS_ERROR;
    return STATUS_OK;
}

status_t enter_usb_mass_storage_mode(TX_BYTE_POOL *pool)
{
    LOG_INFO("USB: Enter mass storage mode - usb otg init\n");
    MX_USB_OTG_FS_PCD_Init();
    LOG_INFO("USB: SD init\n");
    MX_SDMMC1_SD_Init();

    LOG_INFO("USB: Get card info\n");
    HAL_SD_CardInfoTypeDef card_info;
    if (HAL_SD_GetCardInfo(&hsd1, &card_info) != HAL_OK)
    {
        LOG_ERROR("USB MSC: HAL_SD_GetCardInfo failed\n");
        return STATUS_ERROR;
    }

    LOG_INFO("USB: tx_byte_allocate\n");
    UCHAR *ux_pool = NULL;
    if (tx_byte_allocate(pool, (VOID **)&ux_pool, UX_DEVICE_POOL_SIZE, TX_NO_WAIT) != TX_SUCCESS)
    {
        LOG_ERROR("USB MSC: failed to allocate USBX pool\n");
        return STATUS_ERROR;
    }

    LOG_INFO("USB: ux_system_initialize\n");
    if (ux_system_initialize(ux_pool, UX_DEVICE_POOL_SIZE, UX_NULL, 0) != UX_SUCCESS)
    {
        LOG_ERROR("USB MSC: ux_system_initialize failed\n");
        return STATUS_ERROR;
    }

    LOG_INFO("USB: USB framework\n");
    ULONG framework_len, string_len, language_len;
    UCHAR *framework = USBD_Get_Device_Framework_Speed(USBD_FULL_SPEED, &framework_len);
    UCHAR *strings = USBD_Get_String_Framework(&string_len);
    UCHAR *language = USBD_Get_Language_Id_Framework(&language_len);

    LOG_INFO("USB: device stack initalize\n");
    if (ux_device_stack_initialize(UX_NULL, 0, framework, framework_len, strings, string_len,
                                   language, language_len, UX_NULL) != UX_SUCCESS)
    {
        LOG_ERROR("USB MSC: ux_device_stack_initialize failed\n");
        return STATUS_ERROR;
    }

    static UX_SLAVE_CLASS_STORAGE_PARAMETER storage_param;
    storage_param.ux_slave_class_storage_parameter_number_lun = 1;

    UX_SLAVE_CLASS_STORAGE_LUN *lun =
        &storage_param.ux_slave_class_storage_parameter_lun[0];
    lun->ux_slave_class_storage_media_last_lba = card_info.LogBlockNbr - 1;
    lun->ux_slave_class_storage_media_block_length = card_info.LogBlockSize;
    lun->ux_slave_class_storage_media_type = 0; // direct-access block device
    lun->ux_slave_class_storage_media_removable_flag = 0x80; // removable
    lun->ux_slave_class_storage_media_read_only_flag = 0;
    lun->ux_slave_class_storage_media_read = msc_media_read;
    lun->ux_slave_class_storage_media_write = msc_media_write;
    lun->ux_slave_class_storage_media_status = msc_media_status;
    lun->ux_slave_class_storage_media_flush = UX_NULL;

    storage_param.ux_slave_class_storage_parameter_vendor_id =
        (UCHAR *)"SUFST   ";
    storage_param.ux_slave_class_storage_parameter_product_id =
        (UCHAR *)"VCU SD Card     ";
    storage_param.ux_slave_class_storage_parameter_product_rev =
        (UCHAR *)"1.0 ";
    storage_param.ux_slave_class_storage_parameter_product_serial =
        (UCHAR *)"00000001";

    LOG_INFO("USB: device stack class register\n");
    if (ux_device_stack_class_register(_ux_system_slave_class_storage_name, ux_device_class_storage_entry,
                                       1, 0, &storage_param) != UX_SUCCESS)
    {
        LOG_ERROR("USB MSC: ux_device_stack_class_register failed\n");
        return STATUS_ERROR;
    }

    LOG_INFO("USB: dcd_stm32_initialize\n");
    if (ux_dcd_stm32_initialize(0, (ULONG)&hpcd_USB_OTG_FS) != UX_SUCCESS)
    {
        LOG_ERROR("USB MSC: ux_dcd_stm32_initialize failed\n");
        return STATUS_ERROR;
    }

    HAL_PCD_Start(&hpcd_USB_OTG_FS);
    LOG_INFO("USB MSC: active, power cycle to exit\n");

    heartbeat_set_fast();

    while (1)
        tx_thread_sleep(TX_WAIT_FOREVER);

    return STATUS_OK;
}
