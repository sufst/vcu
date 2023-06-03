#include "config.h"

/**
 * @brief   Convert seconds to ticks
 * 
 * @note    The maximum precision is determined by how many ticks there are per
 *          second. E.g. TX_TIMER_TICKS_PER_SECOND = 1000 gives down to 0.001s
 */
#define SECONDS_TO_TICKS(x)  (TX_TIMER_TICKS_PER_SECOND * x)

/**
 * @brief   VCU configuration instance
 * 
 * @details See definitions of parameters in config.h
 *
 * @note    This struct is const, i.e. read only! The config should not change
 *          at runtime.
 *
 * @warning Make sure every single state in the struct is given a value,
 *          otherwise the value will default to zero
 */
static const config_t config_instance = {
    .dash = {
        .run_visual_check = true,     
        .visual_check_all_leds = true,
        .visual_check_ticks = SECONDS_TO_TICKS(2),
        .visual_check_stagger_ticks = SECONDS_TO_TICKS(0.25)
    },
    .ts_activation = {
        .r2d_requires_brake = false,
        .input_active_ticks = SECONDS_TO_TICKS(1),
        .ts_ready_timeout_ticks = SECONDS_TO_TICKS(5),
        .rtds_sound_ticks = SECONDS_TO_TICKS(2.5),
        .ready_wait_led_toggle_ticks = SECONDS_TO_TICKS(0.5)
    }
};

/**
 * @brief   Returns the VCU configuration instance
 */
const config_t* config_get()
{
    return &config_instance;
}