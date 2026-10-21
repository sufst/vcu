/**
 ******************************************************************************
 * @file    sd_diskio.c
 * @brief   FatFs low level disk I/O for the SD card (single drive, pdrv 0)
 ******************************************************************************
 */

#include "diskio.h"
#include "ff.h"
#include "sd_card.h"

static volatile DSTATUS sd_status = STA_NOINIT;

DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv != 0)
    {
        return STA_NOINIT;
    }

    if (!sd_card_is_inserted())
    {
        return STA_NOINIT | STA_NODISK;
    }

    return sd_status;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != 0)
    {
        return STA_NOINIT;
    }

    sd_status = (sd_card_init() == STATUS_OK && sd_card_is_ready()) ? 0 : STA_NOINIT;

    return disk_status(pdrv);
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != 0 || count == 0)
    {
        return RES_PARERR;
    }

    if (sd_status & STA_NOINIT)
    {
        return RES_NOTRDY;
    }

    return (sd_card_read(buff, (uint32_t)sector, count) == STATUS_OK) ? RES_OK : RES_ERROR;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != 0 || count == 0)
    {
        return RES_PARERR;
    }

    if (sd_status & STA_NOINIT)
    {
        return RES_NOTRDY;
    }

    return (sd_card_write(buff, (uint32_t)sector, count) == STATUS_OK) ? RES_OK : RES_ERROR;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (pdrv != 0)
    {
        return RES_PARERR;
    }

    if (sd_status & STA_NOINIT)
    {
        return RES_NOTRDY;
    }

    uint32_t block_count = 0;
    uint32_t block_size = 0;

    switch (cmd)
    {
    case CTRL_SYNC:
        // writes complete (including card programming) before returning
        return RES_OK;

    case GET_SECTOR_COUNT:
        if (sd_card_get_info(&block_count, &block_size) != STATUS_OK)
        {
            return RES_ERROR;
        }
        *(LBA_t *)buff = block_count;
        return RES_OK;

    case GET_SECTOR_SIZE:
        *(WORD *)buff = SD_CARD_BLOCK_SIZE;
        return RES_OK;

    case GET_BLOCK_SIZE:
        // erase block size unknown
        *(DWORD *)buff = 1;
        return RES_OK;

    default:
        return RES_PARERR;
    }
}
