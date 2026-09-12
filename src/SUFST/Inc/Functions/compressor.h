/******************************************************************************
 * @file    compressor.h
 * @brief   Audio-compressor-style soft-knee proportional (+ optional integral)
 *          de-ramp primitive, shared by torque_limiters.c's sub-limiters. The
 *          integral term leaks every call - even while gate_active is false -
 *          and is clamped to a configured ceiling, so it self-bounds and
 *          unwinds once excess drops rather than winding up indefinitely.
 *****************************************************************************/

#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#include <stdbool.h>
#include <stdint.h>

#include "config.h"

typedef struct
{
    const config_compressor_t *config_ptr;
    float integral; // accumulated error, only advances once enabled
} compressor_context_t;

void compressor_init(compressor_context_t *ctx, const config_compressor_t *config_ptr);
uint16_t compressor_apply(compressor_context_t *ctx,
                          uint16_t torque_request,
                          float measured_value,
                          bool gate_active);
void compressor_reset(compressor_context_t *ctx);

#endif
