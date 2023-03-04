/***************************************************************************
 * @file   scs.c
 * @author Tim Brewis (@t-bre)
 * @brief  System critical signals
 ***************************************************************************/

#include "scs.h"

#include "config.h"

/*
 * function prototypes
 */
static uint32_t clip_to_range(uint32_t value, uint32_t min, uint32_t max);
static uint32_t map_adc_reading(scs_t* scs_ptr);

/**
 * @brief           Create new safety critical signal
 *
 * @param[inout]    scs_ptr     Pointer to SCS instance
 * @param[in]       hadc_ptr    Pointer to associated ADC handle
 * @param[in]       adc_min     Minimum expected raw ADC reading
 * @param[in]       adc_max     Maximum expected raw ADC reading
 * @param[in]       min         Minimum mapped value
 * @param[in]       max         Maximum mapped value
 */
void scs_create(scs_t* scs_ptr,
                ADC_HandleTypeDef* hadc_ptr,
                uint32_t adc_min,
                uint32_t adc_max,
                uint32_t min,
                uint32_t max)
{
    if (scs_ptr == NULL || hadc_ptr == NULL)
    {
        scs_ptr->hadc_ptr = NULL;
        return;
    }

    scs_ptr->hadc_ptr = hadc_ptr;
    scs_ptr->adc_min = adc_min;
    scs_ptr->adc_max = adc_max;
    scs_ptr->min = min;
    scs_ptr->max = max;
    scs_ptr->adc_reading = adc_min;
    scs_ptr->mapped_reading = min;

    uint32_t adc_range = adc_max - adc_min;
    uint32_t mapped_range = max - min;

    scs_ptr->scale_up = (adc_range < mapped_range);
    scs_ptr->scale_factor = scs_ptr->scale_up ? mapped_range / adc_range
                                              : adc_range / mapped_range;
    scs_ptr->max_bounds_diff = adc_range * SCS_OUTSIDE_BOUNDS_FRACTION;
}

/**
 * @brief       Reads the ADC and calculates the value of a safety critical
 *              signal
 *
 * @param[in]   scs_ptr     Pointer to SCS instance
 */
uint32_t scs_read(scs_t* scs_ptr)
{
    if (scs_ptr == NULL || scs_ptr->hadc_ptr == NULL)
    {
        return 0;
    }

    if (HAL_ADC_Start(scs_ptr->hadc_ptr) != HAL_OK
        || HAL_ADC_PollForConversion(scs_ptr->hadc_ptr, HAL_MAX_DELAY)
               != HAL_OK)
    {
        scs_ptr->adc_reading = scs_ptr->adc_min;
        scs_ptr->mapped_reading = scs_ptr->min;

        // TODO: use internal fault handling system
        Error_Handler();

        return scs_ptr->mapped_reading;
    }

    scs_ptr->adc_reading = HAL_ADC_GetValue(scs_ptr->hadc_ptr);
    scs_ptr->mapped_reading = map_adc_reading(scs_ptr);

    return scs_ptr->mapped_reading;
}

/**
 * @brief       Validates the ADC reading of a safety critical signal
 *
 * @details     Validation logic:
 *
 *              adc_reading
 *                  ^
 *                  |   invalid
 *                  |   ------------------------- adc_max + max_bounds_diff
 *                  |   valid
 *                  |   ------------------------- adc_max
 *                  |
 *                  |
 *                  |   valid
 *                  |
 *                  |
 *                  |   ------------------------- adc_min
 *                  |   valid
 *                  |   ------------------------- adc_min - max_bounds_diff
 *                  |   invalid
 *
 * @note        scs_read() must be called first
 *
 * @param[in]   scs_ptr     Pointer to SCS instance
 *
 * @return      true        The signal is valid
 * @return      false       The signal is invalid
 */
bool scs_validate(scs_t* scs_ptr)
{
    uint32_t low_diff = 0;
    uint32_t high_diff = 0;

    if (scs_ptr->adc_reading < scs_ptr->adc_min)
    {
        low_diff = scs_ptr->adc_min - scs_ptr->adc_reading;
    }

    if (scs_ptr->adc_reading > scs_ptr->adc_max)
    {
        high_diff = scs_ptr->adc_reading - scs_ptr->adc_max;
    }

    return low_diff < scs_ptr->max_bounds_diff
           && high_diff < scs_ptr->max_bounds_diff;
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

/**
 * @brief       Map raw ADC reading of safety critical signal, with clipping
 *
 * @param[in]   scs_ptr     Pointer to SCS instance
 */
uint32_t map_adc_reading(scs_t* scs_ptr)
{
    uint32_t clipped = clip_to_range(scs_ptr->adc_reading,
                                     scs_ptr->adc_min,
                                     scs_ptr->adc_max);

    uint32_t shifted = clipped - scs_ptr->adc_min;

    uint32_t scaled = scs_ptr->scale_up ? shifted * scs_ptr->scale_factor
                                        : shifted / scs_ptr->scale_factor;

    uint32_t mapped = scaled + scs_ptr->min;

    return clip_to_range(mapped, scs_ptr->min, scs_ptr->max);
}