#include "dash.h"

/*
 * internal functions
 */
static void dash_thread_entry(ULONG input);
static void run_visual_check(uint32_t ticks, bool all_leds, uint32_t stagger);

/**
 * @brief       Initialises the dash service
 *
 * @param[in]   dash_ptr            Dash context
 * @param[in]   stack_pool_ptr      Byte pool to allocate thread stack from
 * @param[in]   thread_config_ptr   Thread configuration
 * @param[in]   vc_config_ptr       Visual check configuration
 */
status_t dash_init(dash_context_t* dash_ptr,
                   TX_BYTE_POOL* stack_pool_ptr,
                   const config_thread_t* thread_config_ptr,
                   const config_vc_t* vc_config_ptr)
{
    status_t status = STATUS_OK;

    dash_ptr->vc_config_ptr = vc_config_ptr;

    // create the thread
    void* stack_ptr = NULL;
    UINT tx_status = tx_byte_allocate(stack_pool_ptr,
                                      &stack_ptr,
                                      thread_config_ptr->stack_size,
                                      TX_NO_WAIT);

    if (tx_status == TX_SUCCESS)
    {
        tx_status = tx_thread_create(&dash_ptr->thread,
                                     (CHAR*) thread_config_ptr->name,
                                     dash_thread_entry,
                                     (ULONG) dash_ptr,
                                     stack_ptr,
                                     thread_config_ptr->stack_size,
                                     thread_config_ptr->priority,
                                     thread_config_ptr->priority,
                                     TX_NO_TIME_SLICE,
                                     TX_AUTO_START);
    }

    UNUSED(tx_status);

    // CubeMX will auto-generate code which sets initial pin states
    // however for ease of editing / readability it's done again here
    HAL_GPIO_WritePin(R2D_LED_GPIO_Port, R2D_LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TS_ON_LED_GPIO_Port, TS_ON_LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DRS_LED_GPIO_Port, DRS_LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(VC_LEDS_GPIO_Port, VC_LEDS_Pin, GPIO_PIN_RESET);

    if (tx_status != TX_SUCCESS)
    {
        status = STATUS_ERROR;
    }

    return status;
}

/**
 * @brief       Dash service thread
 *
 * @param[in]   input   Pointer to dash instance
 */
void dash_thread_entry(ULONG input)
{
    dash_context_t* dash_ptr = (dash_context_t*) input;

    // see 'visible check' rule
    if (dash_ptr->vc_config_ptr->run_check)
    {
        run_visual_check(dash_ptr->vc_config_ptr->led_on_ticks,
                         dash_ptr->vc_config_ptr->all_leds_on,
                         dash_ptr->vc_config_ptr->stagger_ticks);
    }

    // poll inputs

    UNUSED(dash_ptr);
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
