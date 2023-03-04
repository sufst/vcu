/***************************************************************************
 * @file   ready_to_drive.h
 * @author Tim Brewis (@t-bre)
 * @brief  Ready to drive
 ***************************************************************************/

#ifndef READY_TO_DRIVE_H
#define READY_TO_DRIVE_H

#include <stdatomic.h>
#include <stdint.h>

#include "bps.h"

#include <gpio.h>
#include <tx_api.h>

/**
 * @brief   Ready to drive context
 */
typedef struct
{
    GPIO_TypeDef* spkr_gpio_port;
    uint32_t spkr_gpio_pin;
    bool enable_bps_check;

    TX_SEMAPHORE sem;
    TX_TIMER speaker_timer;

} rtd_context_t;

/*
 * function prototypes
 */
void rtd_init(rtd_context_t* rtd_ptr,
              GPIO_TypeDef* spkr_gpio_port,
              uint32_t spkr_gpio_pin,
              uint32_t spkr_on_time,
              bool enable_bps_check);

void rtd_wait(rtd_context_t* rtd_ptr);

void rtd_handle_int(rtd_context_t* rtd_ptr);

#endif
