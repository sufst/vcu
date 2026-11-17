/******************************************************************************
 * @file    trc.h
 * @author  Tim Brewis (@t-bre)
 * @brief   TSAC relay controller (TRC) interface
 *****************************************************************************/

#ifndef TRC_H
#define TRC_H

#include "gpio.h"
#include <stdbool.h>
#include <stdint.h>

#include "status.h"

status_t trc_set_ts_on(GPIO_PinState state);
bool trc_ready(void);

#endif
