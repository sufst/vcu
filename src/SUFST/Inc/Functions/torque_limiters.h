/******************************************************************************
 * @file    torque_limiters.h
 * @brief   Torque de-ramp limiters applied after the torque map: a
 *          slip-ratio-based traction/launch control limiter, and an
 *          inverter DC-bus electrical power de-ramp limiter.
 *****************************************************************************/

#ifndef TORQUE_LIMITERS_H
#define TORQUE_LIMITERS_H

#include <stdbool.h>
#include <stdint.h>

#include "compressor.h"
#include "config.h"

typedef struct
{
    const config_torque_limiters_t *config_ptr;
    float min_denominator_rpm; // effective floor, >= min_motor_speed_rpm / gear_ratio
    compressor_context_t slip;
    compressor_context_t power;
} torque_limiters_context_t;

void torque_limiters_init(torque_limiters_context_t *ctx,
                          const config_torque_limiters_t *config_ptr);

uint16_t torque_limiters_apply(torque_limiters_context_t *ctx,
                               uint16_t torque_request,
                               bool slip_enabled,
                               int16_t motor_rpm,
                               float front_wheel_rpm,
                               float electrical_power_w);

void torque_limiters_reset(torque_limiters_context_t *ctx);

#endif
