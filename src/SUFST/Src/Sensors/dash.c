#include "dash.h"

#include <tx_api.h>

static void
run_visual_check(uint32_t ticks, bool all_leds, uint32_t stagger_ticks);

/**
 * @brief       Initialises the dash and completes the visual check
 *
 * @note        For the visual check to work, this must be called from a thread
 *
 * @param[in]   vc_enable           Enable visual check
 * @param[in]   vc_ticks            Number of ticks for which visual check
 * should last
 * @param[in]   vc_all_leds         Turn on ALL dash LEDs for visual check
 * @param[in]   vc_stagger_ticks    Ticks between turning on each LED
 */
void dash_init(bool vc_enable,
               uint32_t vc_ticks,
               bool vc_all_leds,
               uint32_t vc_stagger_ticks)
{
    // CubeMX will auto-generate code which sets initial pin states
    // however for ease of editing / readability it's done again here
    HAL_GPIO_WritePin(R2D_LED_GPIO_Port, R2D_LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TS_ON_LED_GPIO_Port, TS_ON_LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DRS_LED_GPIO_Port, DRS_LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(VC_LEDS_GPIO_Port, VC_LEDS_Pin, GPIO_PIN_RESET);

    // see 'visible check' rule
    if (vc_enable)
    {
        run_visual_check(vc_ticks, vc_all_leds, vc_stagger_ticks);
    }
}

/**
 * @brief       Runs the 'visual check'
 *
 * @details     Must be called from a thread
 *
 * @param[in]   ticks           Number of ticks to keep all LEDs on
 * @param[in]   all_leds        Turn on all LEDs (rather than just VC_LEDS pin)
 * @param[in]   stagger_ticks   Delay between turning on each LED
 */
void run_visual_check(uint32_t ticks, bool all_leds, uint32_t stagger_ticks)
{
    if (all_leds)
    {
        tx_thread_sleep(stagger_ticks);
        HAL_GPIO_WritePin(TS_ON_LED_GPIO_Port, TS_ON_LED_Pin, GPIO_PIN_SET);
        tx_thread_sleep(stagger_ticks);
        HAL_GPIO_WritePin(R2D_LED_GPIO_Port, R2D_LED_Pin, GPIO_PIN_SET);
        tx_thread_sleep(stagger_ticks);
        HAL_GPIO_WritePin(DRS_LED_GPIO_Port, DRS_LED_Pin, GPIO_PIN_SET);
        tx_thread_sleep(stagger_ticks);
    }

    HAL_GPIO_WritePin(VC_LEDS_GPIO_Port, VC_LEDS_Pin, GPIO_PIN_SET);

    tx_thread_sleep(ticks);

    if (all_leds)
    {
        HAL_GPIO_WritePin(TS_ON_LED_GPIO_Port, TS_ON_LED_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(R2D_LED_GPIO_Port, R2D_LED_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(DRS_LED_GPIO_Port, DRS_LED_Pin, GPIO_PIN_RESET);
    }

    HAL_GPIO_WritePin(VC_LEDS_GPIO_Port, VC_LEDS_Pin, GPIO_PIN_RESET);
}

/**
 * @brief       Sets the state of the R2D LED
 *
 * @param[in]   state   GPIO state
 */
void dash_set_r2d_led_state(GPIO_PinState state)
{
    HAL_GPIO_WritePin(R2D_LED_GPIO_Port, R2D_LED_Pin, state);
}

/**
 * @brief       Sets the state of the TS ON LED
 *
 * @param[in]   state   GPIO state
 */
void dash_set_ts_on_led_state(GPIO_PinState state)
{
    HAL_GPIO_WritePin(TS_ON_LED_GPIO_Port, TS_ON_LED_Pin, state);
}

/**
 * @brief       Sets the state of the DRS LED
 *
 * @param[in]   state   GPIO state
 */
void dash_set_drs_led_state(GPIO_PinState state)
{
    HAL_GPIO_WritePin(DRS_LED_GPIO_Port, DRS_LED_Pin, state);
}
