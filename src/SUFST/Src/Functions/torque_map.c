#include "torque_map.h"

/*
 * internal function prototypes
 */
static inline uint16_t apply_deadzone(torque_map_t* map_ptr, uint16_t input);
static uint16_t null_torque_map(torque_map_t* map_ptr, uint16_t input);
static uint16_t linear_torque_map(torque_map_t* map_ptr, uint16_t input);
static inline uint16_t
apply_speed_limit(torque_map_t* map_ptr, uint16_t input, uint16_t speed);

/**
 * @brief       Initialises the torque map
 *
 * @details     If the mapping function is invalid, a default 'null' torque map
 *              is used where the output is always zero
 *
 * @param[in]   map_ptr     Torque map
 * @param[in]   config_ptr  Configuration
 */
status_t torque_map_init(torque_map_t* map_ptr,
                         const config_torque_map_t* config_ptr)
{
    map_ptr->config_ptr = config_ptr;

    // pre-compute deadzone parameters
    map_ptr->deadzone_end
        = config_ptr->deadzone_fraction * config_ptr->input_max;

    map_ptr->deadzone_scale
        = ((float) config_ptr->input_max)
          / ((float) (config_ptr->input_max - map_ptr->deadzone_end));

    // load mapping function
    status_t status = STATUS_OK;

    switch (config_ptr->function)
    {
    case TORQUE_MAP_LINEAR:
    {
        map_ptr->map_func = linear_torque_map;
        break;
    }

    default:
        map_ptr->map_func = null_torque_map;
        status = STATUS_ERROR;
        break;
    };

    map_ptr->speed_min = config_ptr->speed_min;
    map_ptr->speed_start = config_ptr->speed_start;
    map_ptr->speed_end = config_ptr->speed_end;

    return status;
}

/**
 * @brief       Applies a torque map
 *
 * @param[in]   map_ptr     Torque map
 * @param[in]   input       Input value
 */
uint16_t torque_map_apply(torque_map_t* map_ptr, uint16_t input, uint16_t speed)
{
    const uint16_t input_deadzone = apply_deadzone(map_ptr, input);
    const uint16_t torque = map_ptr->map_func(map_ptr, input_deadzone);
    const uint16_t limited_torque = apply_speed_limit(map_ptr, torque, speed);
    return limited_torque;
}

/**
 * @brief       Applies a deadzone to an input to ensure low input values
 *              result in zero torque
 *
 * @param[in]   map_ptr     Torque map
 * @param[in]   input       Input value
 */
uint16_t apply_deadzone(torque_map_t* map_ptr, uint16_t input)
{
    uint16_t result = 0;

    if (input <= map_ptr->deadzone_end)
    {
        result = 0;
    }
    else
    {
        const uint16_t shifted_input = input - map_ptr->deadzone_end;
        result = (uint16_t) (shifted_input * map_ptr->deadzone_scale);
    }

    return result;
}

/**
 * @brief   A torque map that returns zero
 */
uint16_t null_torque_map(torque_map_t* map_ptr, uint16_t input)
{
    UNUSED(map_ptr);
    UNUSED(input);
    return 0;
}

/**
 * @brief   A linear torque map
 */
uint16_t linear_torque_map(torque_map_t* map_ptr, uint16_t input)
{
    const float scale_factor = map_ptr->config_ptr->output_max
                               / (float) map_ptr->config_ptr->input_max;

    const uint16_t torque = (uint16_t) (input * scale_factor);

    // TODO: clip to range

    return torque;
}

uint16_t
apply_speed_limit(torque_map_t* map_ptr, uint16_t input, uint16_t speed)
{
    uint16_t result = 0;
    if (speed < map_ptr->speed_start)
    {
        result = input;
    }
    else if (speed > map_ptr->speed_end)
    {
        if (input < map_ptr->speed_min)
        {
            result = input;
        }
        else
        {
            result = map_ptr->speed_min;
        }
    }
    else
    {
        uint16_t max_torque
            = map_ptr->config_ptr->output_max
              - (map_ptr->config_ptr->output_max - map_ptr->speed_min)
                    * (speed - map_ptr->speed_start)
                    / (map_ptr->speed_end - map_ptr->speed_start);

        if (input < max_torque)
        {
            result = input;
        }
        else
        {
            result = max_torque;
        }
    }

    return result;
}