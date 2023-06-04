/***************************************************************************
 * @file   bps.h
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief  Brake pressure sensor
 ***************************************************************************/

#ifndef BPS_H
#define BPS_H

#include <stdbool.h>
#include <stdint.h>

#include "config.h"
#include "scs.h"
#include "status.h"

/**
 * @brief   BPS context
 */
typedef struct
{
    scs_t signal;
    uint16_t fully_pressed_threshold;
    const config_bps_t* config_ptr;
} bps_context_t;

/*
 * public functions
 */
status_t bps_init(bps_context_t* bps_ptr, const config_bps_t* config_ptr);
status_t bps_read(bps_context_t* bps_ptr, uint16_t* reading_ptr);
bool bps_fully_pressed(bps_context_t* bps_ptr);

#endif