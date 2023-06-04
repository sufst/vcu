/***************************************************************************
 * @file   scs.c
 * @author Tim Brewis (@t-bre)
 * @brief  System critical signals
 ***************************************************************************/

#include "scs.h"

#include <tx_api.h>

/*
 * internal function prototypes
 */
static uint16_t clip_to_range(uint16_t value, uint16_t min, uint16_t max);
static uint16_t map_adc_reading(scs_t* scs_ptr);
static bool validate(uint16_t adc_reading,
                     uint16_t max_adc_reading,
                     uint16_t min_adc_reading,
                     uint32_t max_diff);

/**
 * @brief       Create new safety critical signal
 *
 * @param[in]   scs_ptr     Pointer to SCS instance
 * @param[in]   config_ptr  Configuration
 */
status_t scs_create(scs_t* scs_ptr, const config_scs_t* config_ptr)
{
    scs_ptr->config_ptr = config_ptr;
    scs_ptr->adc_reading = config_ptr->min_adc;
    scs_ptr->mapped_reading = config_ptr->min_mapped;
    scs_ptr->is_valid = true;
    scs_ptr->invalid_start_tick = 0;

    // compute ranges of raw and mapped readings
    uint32_t adc_range = config_ptr->max_adc - config_ptr->min_adc;
    uint32_t mapped_range = config_ptr->max_mapped - config_ptr->min_mapped;

    // pre-compute the scale factors and validation constants
    scs_ptr->scale_up = (adc_range < mapped_range);
    scs_ptr->scale_factor = scs_ptr->scale_up
                                ? ((float) mapped_range / adc_range)
                                : ((float) adc_range / mapped_range);
    scs_ptr->max_bounds_diff = adc_range * config_ptr->outside_bounds_fraction;

    return STATUS_OK;
}

/**
 * @brief       Reads the SCS (blocking)
 *
 * @details     In the event of an SCS failure, the output is set to the
 *              minimum of the configured mapped range
 *
 * @param[in]   scs_ptr         SCS instance
 * @param[out]  reading_ptr     Output reading, mapped to configured range
 *
 * @retval      STATUS_OK       SCS valid
 * @retval      STATUS_ERROR    SCS out of bounds, or HAL error
 */
status_t scs_read(scs_t* scs_ptr, uint16_t* reading_ptr)
{
    status_t status = STATUS_OK;
    ADC_HandleTypeDef* hadc = scs_ptr->config_ptr->hadc;

    // read from the ADC and validate
    if ((HAL_ADC_Start(hadc) == HAL_OK)
        && (HAL_ADC_PollForConversion(hadc, HAL_MAX_DELAY) == HAL_OK))
    {
        scs_ptr->adc_reading = HAL_ADC_GetValue(hadc);
        scs_ptr->is_valid = validate(scs_ptr->adc_reading,
                                     scs_ptr->config_ptr->max_adc,
                                     scs_ptr->config_ptr->min_adc,
                                     scs_ptr->max_bounds_diff);

        if (scs_ptr->is_valid)
        {
            // everything is ok
            status = STATUS_OK;
            scs_ptr->invalid_start_tick = 0;
            scs_ptr->mapped_reading = map_adc_reading(scs_ptr);
            *reading_ptr = scs_ptr->mapped_reading;
        }
        else
        {
            status = STATUS_ERROR;
        }
    }
    else
    {
        // TODO: HAL errors here behave as SCS errors, consider if they should
        //       be treated differently for diagnostics
        status = STATUS_ERROR;
    }

    if (status != STATUS_OK)
    {
        scs_ptr->invalid_start_tick = tx_time_get();
        scs_ptr->is_valid = false;
        scs_ptr->adc_reading = scs_ptr->config_ptr->min_adc;
        scs_ptr->mapped_reading = scs_ptr->config_ptr->min_mapped;
    }

    return status;
}

/**
 * @brief       Map raw ADC reading of safety critical signal, with clipping
 *
 * @param[in]   scs_ptr     Pointer to SCS instance
 */
uint16_t map_adc_reading(scs_t* scs_ptr)
{
    uint16_t clipped = clip_to_range(scs_ptr->adc_reading,
                                     scs_ptr->config_ptr->min_adc,
                                     scs_ptr->config_ptr->max_adc);

    uint16_t shifted = clipped - scs_ptr->config_ptr->min_adc;

    uint16_t scaled = (scs_ptr->scale_up) ? shifted * scs_ptr->scale_factor
                                          : shifted / scs_ptr->scale_factor;

    uint16_t mapped = scaled + scs_ptr->config_ptr->min_mapped;

    return clip_to_range(mapped,
                         scs_ptr->config_ptr->min_mapped,
                         scs_ptr->config_ptr->max_mapped);
}

/**
 * @brief       Clips a value to a range
 *
 * @details     Value is clipped to [min, max]
 *
 * @param[in]   value   Value to clip
 * @param[in]   min     Minimum of range
 * @param[in]   max     Maximum of range
 */
uint16_t clip_to_range(uint16_t value, uint16_t min, uint16_t max)
{
    uint16_t ret = value;

    if (value < min)
    {
        ret = min;
    }
    else if (value > max)
    {
        ret = max;
    }

    return ret;
}

/**
 * @brief       Validates the ADC reading of a safety critical signal
 *
 * @details     Validation logic:
 *
 *              adc_reading
 *                  ^
 *                  |   invalid
 *                  |   ------------------------- max + max_diff
 *                  |   valid
 *                  |   ------------------------- max
 *                  |
 *                  |
 *                  |   valid
 *                  |
 *                  |
 *                  |   ------------------------- min
 *                  |   valid
 *                  |   ------------------------- min - max_diff
 *                  |   invalid
 *
 *
 * @param[in]   adc_reading         ADC reading
 * @param[in]   max_adc_reading     Maximum valid ADC reading
 * @param[in]   min_adc_reading     Minimum valid ADC reading
 * @param[in]   max_diff            Maximum amount over max/min
 *
 * @return      true                The signal is valid
 * @return      false               The signal is invalid
 */
bool validate(uint16_t adc_reading,
              uint16_t max_adc_reading,
              uint16_t min_adc_reading,
              uint32_t max_diff)
{
    uint16_t low_diff = 0;
    uint16_t high_diff = 0;

    if (adc_reading < min_adc_reading)
    {
        low_diff = min_adc_reading - adc_reading;
    }

    if (adc_reading > max_adc_reading)
    {
        high_diff = adc_reading - max_adc_reading;
    }

    return (low_diff < max_diff) && (high_diff < max_diff);
}