#include "dash.h"

#include <tx_api.h>

static void
run_visual_check(uint32_t ticks, bool all_leds, uint32_t stagger_ticks);

/**
 * @brief       Initialises the dash and completes the visual check
 *
 * @note        For the visual check to work, this must be called from a thread
 *
 * @param[in]   vc_config   Visual check configuration
 */
void dash_init(const config_vc_t* vc_config_ptr)
{
    // CubeMX will auto-generate code which sets initial pin states
    // however for ease of editing / readability it's done again here
    HAL_GPIO_WritePin(R2D_LED_GPIO_Port, R2D_LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TS_ON_LED_GPIO_Port, TS_ON_LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DRS_LED_GPIO_Port, DRS_LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(VC_LEDS_GPIO_Port, VC_LEDS_Pin, GPIO_PIN_RESET);

    // see 'visible check' rule
    if (vc_config_ptr->run_check)
    {
        run_visual_check(vc_config_ptr->led_on_ticks,
                         vc_config_ptr->all_leds_on,
                         vc_config_ptr->stagger_ticks);
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
 * @brief       Sets the state of the TS on LED
 *
 * @param[in]   state   GPIO state
 */
void dash_set_ts_on_led_state(GPIO_PinState state)
{
    HAL_GPIO_WritePin(TS_ON_LED_GPIO_Port, TS_ON_LED_Pin, state);
}

/**
 * @brief   Toggles the TS on LED
 */
void dash_toggle_ts_on_led()
{
    HAL_GPIO_TogglePin(TS_ON_LED_GPIO_Port, TS_ON_LED_Pin);
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

/**
 * @brief   Returns the state of the R2D button
 */
bool dash_get_r2d_btn_state()
{
    return (HAL_GPIO_ReadPin(R2D_BTN_GPIO_Port, R2D_BTN_Pin) == GPIO_PIN_SET);
}

/**
 * @brief   Returns the state of the TS on button
 */
bool dash_get_ts_on_btn_state()
{
    return (HAL_GPIO_ReadPin(TS_ON_BTN_GPIO_Port, TS_ON_BTN_Pin)
            == GPIO_PIN_SET);
}

/**
 * @brief   Returns the state of the DRS button
 */
bool dash_get_drs_btn_state()
{
    return (HAL_GPIO_ReadPin(DRS_BTN_GPIO_Port, DRS_BTN_Pin) == GPIO_PIN_SET);
}
