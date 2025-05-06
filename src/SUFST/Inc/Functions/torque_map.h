/******************************************************************************
 * @file    torque_map.h
 * @author  Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief   Torque map for converting APPS readings into torque requests
 * @note    In future, the torque map could be pre-computed as a look up table
 *          to avoid floating point calculations and improve performance
 *****************************************************************************/

#ifndef TORQUE_MAP_H
#define TORQUE_MAP_H

#include <stdint.h>

#include "config.h"
#include "status.h"
#include "torque_map_funcs.h"

/**
 * @brief   Torque map context
 */
typedef struct _torque_map_t
{
    uint16_t (*map_func)(struct _torque_map_t*, uint16_t); // mapping function
    uint16_t deadzone_end;                                 // end of deadzone
    float deadzone_scale;                  // scale factor for inputs
    const config_torque_map_t* config_ptr; // configuration
    uint16_t
        limit_min; // minimum Torque request at the end of the limit (Nm *10)
    uint8_t limit_start; // BMS temp to start limiting torque (Celcius)
    uint8_t limit_end;   // BMS temp for maximum toque limiting (Celcius)
} torque_map_t;

/*
 * public functions
 */
status_t torque_map_init(torque_map_t* map_ptr,
                         const config_torque_map_t* config_ptr);
uint16_t
torque_map_apply(torque_map_t* map_ptr, uint16_t input, uint8_t bms_temp);

#endif