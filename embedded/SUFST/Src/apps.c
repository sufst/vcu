/***************************************************************************
 * @file   apps.c
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-06-29
 * @brief  APPS input implementation
 ***************************************************************************/

#include "apps.h"

#include <stdbool.h>

#include "config.h"

#include "fault.h"
#include "trace.h"

#include "adc.h"

/*
 * function prototypes
 */
static void read_adcs(uint32_t readings[2]);
static uint32_t map_adc_reading(uint32_t reading, uint32_t min, uint32_t max);
static uint32_t clip_to_range(uint32_t value, uint32_t min, uint32_t max);

#if !APPS_DISABLE_BOUNDS_CHECK
static void validate_raw_reading(uint32_t* reading_ptr, uint32_t min, uint32_t max);
#endif

#if !APPS_DISABLE_DIFF_CHECK
static void validate_mapped_readings(uint32_t readings[2]);
#endif

/**
 * @brief   Reads the APPS input
 *
 * @details The ADCs have a resolution of 16 bits which is then scaled down to
 *          10 bits
 */
uint32_t read_apps()
{
    uint32_t apps_inputs[2];

    read_adcs(apps_inputs);

#if !APPS_DISABLE_BOUNDS_CHECK
    validate_raw_reading(&apps_inputs[0], APPS_1_ADC_MIN, APPS_1_ADC_MAX);
    validate_raw_reading(&apps_inputs[1], APPS_2_ADC_MIN, APPS_2_ADC_MAX);
#endif

    apps_inputs[0]
        = map_adc_reading(apps_inputs[0], APPS_1_ADC_MIN, APPS_1_ADC_MAX);
    apps_inputs[1]
        = map_adc_reading(apps_inputs[1], APPS_2_ADC_MIN, APPS_2_ADC_MAX);

#if !APPS_DISABLE_DIFF_CHECK
    validate_mapped_readings(apps_inputs);
#endif

    uint32_t average_input = (apps_inputs[0] + apps_inputs[1]) / 2;

    trace_log_event(TRACE_APPS_INPUT_EVENT, (ULONG) average_input, 0, 0, 0);

    return average_input;
}

/**
 * @brief       Read APPS ADCs (raw)
 *
 * @details     APPS 1: hadc1 (PA3)
 *              APPS 2: hadc2 (PB1)
 *
 * @param[out]  readings    Array of two raw ADC readings
 */
void read_adcs(uint32_t readings[2])
{
    if (HAL_ADC_Start(&hadc1) != HAL_OK || HAL_ADC_Start(&hadc2) != HAL_OK
        || HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY) != HAL_OK
        || HAL_ADC_PollForConversion(&hadc2, HAL_MAX_DELAY) != HAL_OK)
    {
        critical_fault(CRITICAL_FAULT_HAL);

        readings[0] = 0;
        readings[1] = 0;

        return;
    }

    readings[0] = HAL_ADC_GetValue(&hadc1);
    readings[1] = HAL_ADC_GetValue(&hadc2);
}

/**
 * @brief           Maps a raw ADC reading to the required range and resolution
 *                  for APPS inputs
 *
 * @param[inout]    raw_reading     Raw ADC reading
 * @param[in]       min             Minimum expected raw reading
 * @param[in]       max             Maximum expected raw reading
 */
uint32_t map_adc_reading(uint32_t raw_reading, uint32_t min, uint32_t max)
{
    uint32_t clipped_reading = clip_to_range(raw_reading, min, max);

    static const uint32_t apps_max = (1 << APPS_SCALED_RESOLUTION) - 1;
    const uint32_t scale_factor = (max - min) / apps_max;

    uint32_t mapped_reading = (clipped_reading - min) / scale_factor;
    mapped_reading = clip_to_range(mapped_reading, 0, apps_max);

    return mapped_reading;
}

#if !APPS_DISABLE_BOUNDS_CHECK
/**
 * @brief           Validates a raw APPS ADC reading and triggers a fault if
 *                  it is invalid
 *
 * @details         Readings will be set to 0 if a fault is triggered
 *
 * @param[inout]    reading     Raw APPS ADC reading
 */
void validate_raw_reading(uint32_t* reading_ptr, uint32_t min, uint32_t max)
{
    static const uint32_t max_diff
        = APPS_OUTSIDE_BOUNDS_FRACTION * ((1 << APPS_ADC_RESOLUTION) - 1);

    uint32_t low_diff = 0;
    uint32_t high_diff = 0;

    if (*reading_ptr < min)
    {
        low_diff = min - *reading_ptr;
    }

    if (*reading_ptr > max)
    {
        high_diff = *reading_ptr - max;
    }
    
    if (low_diff > max_diff || high_diff > max_diff)
    {
        critical_fault(CRITICAL_FAULT_APPS_INPUT_OUTSIDE_BOUNDS);

        *reading_ptr = 0;
    }
}
#endif

#if !APPS_DISABLE_DIFF_CHECK
/**
 * @brief           Validates the mapped APPS readings and triggers a fault if
 *                  they are invalid
 *
 * @details         Readings will be set to 0 if a fault is triggered
 *
 * @param[inout]    readings    Array of two mapped APPS readings
 */
void validate_mapped_readings(uint32_t readings[2])
{
    static const uint32_t max_diff
        = APPS_MAX_DIFF_FRACTION * ((1 << APPS_SCALED_RESOLUTION) - 1);

    uint32_t diff = (readings[1] > readings[0]) ? (readings[1] - readings[0])
                                                : (readings[0] - readings[1]);

    if (diff > max_diff)
    {
        critical_fault(CRITICAL_FAULT_APPS_INPUT_DISCREPANCY);

        readings[0] = 0;
        readings[1] = 0;
    }
}
#endif

/**
 * @brief       Clips a value within a range
 *
 * @details     Value is clipped to [min, max]
 *
 * @param[in]   value   Value to clip
 * @param[in]   min     Minimum of range
 * @param[in]   max     Maximum of range
 */
uint32_t clip_to_range(uint32_t value, uint32_t min, uint32_t max)
{
    if (value < min)
    {
        return min;
    }
    else if (value > max)
    {
        return max;
    }

    return value;
}