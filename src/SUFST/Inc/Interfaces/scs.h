/***************************************************************************
 * @file   scs.h
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief  System critical signals
 ***************************************************************************/

#ifndef SCS_H
#define SCS_H

#include <adc.h>
#include <stdbool.h>
#include <stdint.h>

#include "config.h"
#include "status.h"

/*
 * error codes, unique to instances within the VCU
 */
#define SCS_ERROR_NONE             0x00
#define SCS_ERROR_APPS1            0x01
#define SCS_ERROR_APPS2            0x02
#define SCS_ERROR_APPS_DISCREPANCY 0x04
#define SCS_ERROR_BPS              0x08

/**
 * @brief Analogue system critical signal
 */
typedef struct
{
    uint16_t adc_reading;           // latest raw ADC reading
    uint16_t mapped_reading;        // latest mapped reading
    bool is_valid;                  // true if signal is within bounds
    uint32_t invalid_start_tick;    // tick at which signal became invalid
    bool scale_up;                  // flag indicating scaling up
    float scale_factor;             // cached scale factor for mapping
    uint32_t max_bounds_diff;       // cached bounds difference for validation
    const config_scs_t* config_ptr; // configuration

} scs_t;

/*
 * function prototypes
 */
status_t scs_create(scs_t* scs_ptr, const config_scs_t* config_ptr);
status_t scs_read(scs_t* scs_ptr, uint16_t* reading_ptr);
uint16_t get_adc(scs_t* scs_ptr);

#endif