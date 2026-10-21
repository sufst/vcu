/**
 ******************************************************************************
 * @file    usbd_conf.h
 * @brief   USB device library configuration (USB OTG FS, mass storage class)
 ******************************************************************************
 */

#ifndef __USBD_CONF__H__
#define __USBD_CONF__H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "stm32f7xx.h"
#include "stm32f7xx_hal.h"

#define USBD_MAX_NUM_INTERFACES     1U
#define USBD_MAX_NUM_CONFIGURATION  1U
#define USBD_MAX_STR_DESC_SIZ       512U
#define USBD_DEBUG_LEVEL            0U
#define USBD_LPM_ENABLED            0U
#define USBD_SELF_POWERED           1U

/* MSC class: bytes transferred per SD card access (multiple of 512) */
#define MSC_MEDIA_PACKET            4096U

#define DEVICE_FS 0
#define DEVICE_HS 1

/* memory management: static allocation only */
#define USBD_malloc (void *)USBD_static_malloc
#define USBD_free   USBD_static_free
#define USBD_memset memset
#define USBD_memcpy memcpy
#define USBD_Delay  HAL_Delay

/* debug logging disabled */
#define USBD_UsrLog(...) do {} while (0)
#define USBD_ErrLog(...) do {} while (0)
#define USBD_DbgLog(...) do {} while (0)

void *USBD_static_malloc(uint32_t size);
void USBD_static_free(void *p);

#ifdef __cplusplus
}
#endif

#endif /* __USBD_CONF__H__ */
