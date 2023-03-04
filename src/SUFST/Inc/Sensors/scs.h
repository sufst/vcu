/***************************************************************************
 * @file   scs.h
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief  System critical signals
 ***************************************************************************/

#ifndef SCS_H
#define SCS_H

#include <stdbool.h>
#include <stdint.h>

#include "adc.h"

/**
 * @brief Analogue system critical signal
 */
typedef struct
{
    /**
     * @brief Pointer to associated ADC handle
     */
    ADC_HandleTypeDef* hadc_ptr;

    /**
     * @brief Minimum expected raw ADC reading
     */
    uint32_t adc_min;

    /**
     * @brief Maximum expected raw ADC reading
     */
    uint32_t adc_max;

    /**
     * @brief Minimum value once mapped
     */
    uint32_t min;

    /**
     * @brief Maximum value once mapped
     */
    uint32_t max;

    /**
     * @brief Cached scale factor for mapping
     */
    uint32_t scale_factor;

    /**
     * @brief Cached max bounds difference for validation
     */
    uint32_t max_bounds_diff;

    /**
     * @brief Flag to scale up or down when mapping
     */
    bool scale_up;

    /**
     * @brief Latest raw ADC reading
     */
    uint32_t adc_reading;

    /**
     * @brief Latest reading mapped to range [min, max]
     */
    uint32_t mapped_reading;

} scs_t;

/*
 * function prototypes
 */
void scs_create(scs_t* scs_ptr,
                ADC_HandleTypeDef* hadc_ptr,
                uint32_t adc_min,
                uint32_t adc_max,
                uint32_t min,
                uint32_t max);
uint32_t scs_read(scs_t* scs_ptr);
bool scs_validate(scs_t* scs_ptr);

#endif