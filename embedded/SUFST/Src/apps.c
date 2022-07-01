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
static void scale_adc_readings(uint32_t readings[2]);

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
    scale_adc_readings(readings);

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
 * @details     APPS1: hadc1 (PA3)
 *              APPS2: hadc2 (PB1)
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
 * @brief           Scale raw ADC readings
 *
 * @param[inout]    readings    Array of two raw ADC readings
 */
void scale_adc_readings(uint32_t readings[2])
{
    const int shift = APPS_ADC_RESOLUTION - APPS_SCALED_RESOLUTION;

    for (int i = 0; i < 2; i++)
    {
        readings[i] >>= shift;
    }
}