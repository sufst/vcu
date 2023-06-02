#include "config.h"

/**
 * @brief   VCU configuration instance
 *
 * @details The intended usage is that the main VCU module loads this struct
 *          and initialises all other modules based on its state. Theoretically
 *          all the individual modules could load the global config, but that
 *          makes them less portable / decoupled.
 *
 * @note    This struct is const, i.e. read only!
 *
 * @warning Make sure every single state in the struct is given a value,
 *          otherwise the value will default to zero
 */
static const config_t config_instance = {
    .dash = {.run_visual_check = true,
             .visual_check_all_leds = true,
             .visual_check_ticks = (TX_TIMER_TICKS_PER_SECOND * 2),
             .visual_check_stagger_ticks = (TX_TIMER_TICKS_PER_SECOND * 0.25)}};

/**
 * @brief   Returns the VCU configuration instance
 */
const config_t* config_get()
{
    return &config_instance;
}