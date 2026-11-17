/***************************************************************************
 * @file   sd_card.h
 * @brief  SD card block device (SDMMC1 + DMA)
 * @details Shared by the FatFS disk driver (SD logging) and the USB mass
 *          storage mode. The two are never active at the same time, so no
 *          locking is done here.
 ***************************************************************************/

#ifndef SD_CARD_H
#define SD_CARD_H

#include <stdbool.h>
#include <stdint.h>

#include "status.h"

#define SD_CARD_BLOCK_SIZE 512U

bool sd_card_is_inserted(void);
status_t sd_card_init(void);
bool sd_card_is_ready(void);
status_t sd_card_get_info(uint32_t *block_count, uint32_t *block_size);
status_t sd_card_read(uint8_t *buf, uint32_t block, uint32_t count);
status_t sd_card_write(const uint8_t *buf, uint32_t block, uint32_t count);

#endif
