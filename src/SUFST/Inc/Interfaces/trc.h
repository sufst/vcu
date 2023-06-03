/******************************************************************************
 * @file    trc.h
 * @author  Tim Brewis (@t-bre)
 * @brief   TSAC relay controller (TRC) interface
 *****************************************************************************/

#ifndef TRC_H
#define TRC_H

#include <gpio.h>
#include <stdint.h>

#include "status.h"

status_t trc_set_ts_on(GPIO_PinState state);
status_t trc_wait_for_ready(uint32_t poll_ticks, uint32_t timeout);

#endif