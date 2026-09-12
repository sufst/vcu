#include "torque_map.h"

#include <math.h>

/*
 * internal function prototypes
 */
static inline uint16_t apply_deadzone(const torque_map_t *map_ptr, uint16_t input);
static uint16_t null_torque_map(torque_map_t *map_ptr, uint16_t input);
static uint16_t linear_torque_map(torque_map_t *map_ptr, uint16_t input);
static uint16_t exponential_torque_map(torque_map_t *map_ptr, uint16_t input);

/**
 * @brief       Initialises the torque map
 *
 * @details     If the mapping function is invalid, a default 'null' torque map
 *              is used where the output is always zero
 *
 * @param[in]   map_ptr     Torque map
 * @param[in]   config_ptr  Configuration
 */
status_t torque_map_init(torque_map_t *map_ptr, const config_torque_map_t *config_ptr)
{
    map_ptr->config_ptr = config_ptr;

    // pre-compute deadzone parameters
    map_ptr->deadzone_end = config_ptr->deadzone_fraction * config_ptr->input_max;

    map_ptr->deadzone_scale = ((float)config_ptr->input_max) /
        ((float)(config_ptr->input_max - map_ptr->deadzone_end));
    
    map_ptr->output_max = config_ptr->output_max;
    
    switch (config_ptr->function)
    {
        case TORQUE_MAP_LINEAR:
            map_ptr->map_func = linear_torque_map;
            break;
        case TORQUE_MAP_EXPONENTIAL:
            map_ptr->map_func = exponential_torque_map;
            break;
        default:
            map_ptr->map_func = null_torque_map;
            return STATUS_ERROR;
    };

    return STATUS_OK;
}

/**
 * @brief       Applies a torque map
 *
 * @param[in]   map_ptr     Torque map
 * @param[in]   input       Input value
 */
uint16_t torque_map_apply(torque_map_t *map_ptr, uint16_t input)
{
    return map_ptr->map_func(map_ptr, apply_deadzone(map_ptr, input));
}

/**
 * @brief       Sets the torque ceiling for the active drive mode
 *
 * @param[in]   map_ptr     Torque map
 * @param[in]   output_max  Torque ceiling for this mode (Nm * 10)
 */
void torque_map_set_output_max(torque_map_t *map_ptr, uint16_t output_max)
{
    map_ptr->output_max = output_max;
}

/**
 * @brief       Applies a deadzone to an input to ensure low input values
 *              result in zero torque
 *
 * @param[in]   map_ptr     Torque map
 * @param[in]   input       Input value
 */
uint16_t apply_deadzone(const torque_map_t *map_ptr, uint16_t input)
{
    if (input <= map_ptr->deadzone_end)
    {
        return 0;
    }
    else
    {
        const uint16_t shifted_input = input - map_ptr->deadzone_end;
        return (uint16_t)(shifted_input * map_ptr->deadzone_scale);
    }
}

/**
 * @brief   A torque map that returns zero
 */
uint16_t null_torque_map(torque_map_t *map_ptr, uint16_t input)
{
    UNUSED(map_ptr);
    UNUSED(input);
    return 0;
}

/**
 * @brief   A linear torque map
 */
uint16_t linear_torque_map(torque_map_t *map_ptr, uint16_t input)
{
    const float scale_factor = map_ptr->output_max / (float)map_ptr->config_ptr->input_max;

    const uint16_t torque = (uint16_t)(input * scale_factor);

    return torque;
}

/**
 * @brief   (input/input_max)^exponent torque map
 */
uint16_t exponential_torque_map(torque_map_t *map_ptr, uint16_t input)
{
    if (input > map_ptr->config_ptr->input_max)
    {
        input = map_ptr->config_ptr->input_max;
    }

    const float fraction = (float)input / (float)map_ptr->config_ptr->input_max;
    const float shaped = powf(fraction, map_ptr->config_ptr->exponent);

    return (uint16_t)(shaped * map_ptr->output_max);
}
