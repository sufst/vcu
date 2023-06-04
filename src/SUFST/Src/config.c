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
        .thread = {
            .name = "Dash",
            .priority = 4,
            .stack_size = 1024
        },
        .btn_active_ticks = SECONDS_TO_TICKS(1),
        .btn_sample_ticks = SECONDS_TO_TICKS(0.1),
        .vc_run_check = true,     
        .vc_all_leds_on = true,
        .vc_led_on_ticks = SECONDS_TO_TICKS(2),
        .vc_stagger_ticks = SECONDS_TO_TICKS(0.25)
    },
    .ctrl = {
        .thread = {
            .name = "Control",
            .priority = 2,
            .stack_size = 1024
        },
        .r2d_requires_brake = false,
        .ts_ready_poll_ticks = SECONDS_TO_TICKS(0.1),
        .ts_ready_timeout_ticks = SECONDS_TO_TICKS(5),
        .precharge_timeout_ticks = SECONDS_TO_TICKS(5),
        .rtds_sound_ticks = SECONDS_TO_TICKS(2.5),
        .ready_wait_led_toggle_ticks = SECONDS_TO_TICKS(0.5),
        .error_led_toggle_ticks = SECONDS_TO_TICKS(0.1)
    },
    .canbc = {
        .thread = {
            .name = "CANBC",
            .priority = 4,
            .stack_size = 1024
        },
        .broadcast_period_ticks = SECONDS_TO_TICKS(0.1)
    }
};

/**
 * @brief   Returns the VCU configuration instance
 */
const config_t* config_get()
{
    return &config_instance;
}