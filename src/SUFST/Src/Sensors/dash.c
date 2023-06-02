#include "dash.h"

#include <tx_api.h>

static void run_visual_check(uint32_t ticks);

/**
 * @brief       Initialises the dash and completes the visual check
 *
 * @note        For the visual check to work, this must be called from a thread
 *
 * @param[in]   vc_ticks    Number of ticks for which visual check should last
 */
void dash_init(uint32_t vc_ticks)
{
    // CubeMX will auto-generate code which sets initial pin states
    // however for ease of editing / readability it's done again here
    HAL_GPIO_WritePin(R2D_LED_GPIO_Port, R2D_LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TS_ON_LED_GPIO_Port, TS_ON_LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DRS_LED_GPIO_Port, DRS_LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(VC_LEDS_GPIO_Port, VC_LEDS_Pin, GPIO_PIN_RESET);

    // see 'visible check' rule
    run_visual_check(vc_ticks);
}

/**
 * @brief   Runs the 'visual check'
 *
 * @details Must be called from a thread
 */
void run_visual_check(uint32_t ticks)
{
    // TODO: include the option to only have VC LEDs pin be set
    HAL_GPIO_WritePin(VC_LEDS_GPIO_Port, VC_LEDS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(R2D_LED_GPIO_Port, R2D_LED_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TS_ON_LED_GPIO_Port, TS_ON_LED_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(DRS_LED_GPIO_Port, DRS_LED_Pin, GPIO_PIN_SET);

    tx_thread_sleep(ticks);

    HAL_GPIO_WritePin(VC_LEDS_GPIO_Port, VC_LEDS_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(R2D_LED_GPIO_Port, R2D_LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TS_ON_LED_GPIO_Port, TS_ON_LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DRS_LED_GPIO_Port, DRS_LED_Pin, GPIO_PIN_RESET);
}

void dash_set_r2d_led_state(GPIO_PinState state)
{
}

void dash_set_ts_on_led_state(GPIO_PinState state)
{
}

void dash_set_vc_leds_state(GPIO_PinState state)
{
}

void dash_set_drs_led_state(GPIO_PinState state)
{
}
