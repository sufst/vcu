#include "clip_to_range.h"

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