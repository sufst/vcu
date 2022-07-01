/***************************************************************************
 * @file   apps.c
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-06-29
 * @brief  APPS input implementation
 ***************************************************************************/

#include "apps.h"

#include "config.h"

#include "fault.h"
#include "trace.h"

#include "adc.h"

/*
 * function prototypes
 */
static void read_adcs(uint32_t readings[2]);
static void map_adc_reading(uint32_t* reading, uint32_t min, uint32_t max);

static uint32_t clip_to_range(uint32_t value, uint32_t min, uint32_t max);

/**
 * @brief   Reads the APPS input
 *
 * @details The ADCs have a resolution of 16 bits which is then scaled down to
 *          10 bits
 */
uint32_t read_apps()
{
    uint32_t readings[2];

    read_adcs(readings);

    map_adc_reading(&readings[0], APPS_1_ADC_MIN, APPS_1_ADC_MAX);
    map_adc_reading(&readings[1], APPS_2_ADC_MIN, APPS_2_ADC_MAX);

    uint32_t average_reading = (readings[0] + readings[1]) / 2;

    trace_log_event(TRACE_APPS_INPUT_EVENT,
                    (ULONG) average_reading,
                    0,
                    0,
                    0);

    return average_reading;
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
 * @param[inout]    reading     Pointer to raw ADC reading
 * @param[in]       min         Minimum expected raw reading
 * @param[in]       max         Maximum expected raw reading
 */
void map_adc_reading(uint32_t* reading, uint32_t min, uint32_t max)
{
    uint32_t clipped = clip_to_range(*reading, min, max);

    const uint32_t apps_max = (1 << APPS_SCALED_RESOLUTION) - 1;
    const uint32_t scale_factor = (max - min) / apps_max;

    *reading = (clipped - min) / scale_factor;
    *reading = clip_to_range(*reading, 0, apps_max);
}

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