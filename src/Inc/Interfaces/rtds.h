/***************************************************************************
 * @file   rtds.h
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief  Ready to drive speaker
 ***************************************************************************/

#ifndef RTDS_H
#define RTDS_H

#include <FreeRTOS.h>
#include <stdbool.h>
#include <stdint.h>

#include "config.h"
#include "log.h"
#include "status.h"

void rtds_set(const config_rtds_t *config_ptr, bool on);

/**
 * @brief   Non-blocking RTDS pulse state
 */
typedef struct
{
    const config_rtds_t *config_ptr; // RTDS configuration
    bool active;                     // whether the pulse is currently running
    bool pin_on;                     // current RTDS GPIO output state
    TickType_t phase_start; // tick timestamp the current on/off phase began
} rtds_pulse_context_t;

void rtds_pulse_init(rtds_pulse_context_t *pulse_ptr, const config_rtds_t *config_ptr);
void rtds_pulse_tick(rtds_pulse_context_t *pulse_ptr, bool enable);

#endif