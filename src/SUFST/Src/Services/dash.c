#include "dash.h"

/*
 * internal functions
 */
static void dash_thread_entry(ULONG input);
static void run_visual_check(uint32_t ticks);
static void toggle_ts_on_callback(ULONG input);
static status_t input_wait(input_context_t *input_ptr);

/**
 * @brief       Initialises the dash service
 *
 * @param[in]   dash_ptr            Dash context
 * @param[in]   stack_pool_ptr      Byte pool to allocate thread stack from
 * @param[in]   config_ptr          Dash configuration
 */
status_t dash_init(dash_context_t *dash_ptr,
                   TX_BYTE_POOL *stack_pool_ptr,
                   const config_dash_t *config_ptr)
{
    status_t status = STATUS_OK;

    dash_ptr->config_ptr = config_ptr;
    dash_ptr->r2d_flag = false;
    dash_ptr->tson_flag = false;

    dash_ptr->r2d = (input_context_t){
        .port = R2D_BTN_GPIO_Port,
        .pin = R2D_BTN_Pin,
        .active_state = true,
        .last_state = false,
        .require_release = true,
        .active_start = 0,
        .required_ticks = dash_ptr->config_ptr->btn_active_ticks,
        .sample_ticks = dash_ptr->config_ptr->btn_sample_ticks};

    dash_ptr->tson = (input_context_t){
        .port = TS_ON_BTN_GPIO_Port,
        .pin = TS_ON_BTN_Pin,
        .active_state = true,
        .last_state = false,
        .require_release = true,
        .active_start = 0,
        .required_ticks = dash_ptr->config_ptr->btn_active_ticks,
        .sample_ticks = dash_ptr->config_ptr->btn_sample_ticks};

    // create the thread
    void *stack_ptr = NULL;
    UINT tx_status = tx_byte_allocate(stack_pool_ptr,
                                      &stack_ptr,
                                      config_ptr->thread.stack_size,
                                      TX_NO_WAIT);

    if (tx_status == TX_SUCCESS)
    {
        tx_status = tx_thread_create(&dash_ptr->thread,
                                     (CHAR *)config_ptr->thread.name,
                                     dash_thread_entry,
                                     (ULONG)dash_ptr,
                                     stack_ptr,
                                     config_ptr->thread.stack_size,
                                     config_ptr->thread.priority,
                                     config_ptr->thread.priority,
                                     TX_NO_TIME_SLICE,
                                     TX_AUTO_START);
    }

    if (tx_status != TX_SUCCESS)
    {
        status = STATUS_ERROR;
    }

    // create timer for toggling TS on LED
    tx_status = tx_timer_create(&dash_ptr->ts_on_toggle_timer,
                                NULL,
                                toggle_ts_on_callback,
                                (ULONG)dash_ptr,
                                TX_WAIT_FOREVER,
                                TX_WAIT_FOREVER,
                                TX_NO_ACTIVATE);

    if (tx_status != TX_SUCCESS)
    {
        status = STATUS_ERROR;
    }

    // CubeMX will auto-generate code which sets initial pin states
    // however for ease of editing / readability it's done again here
    HAL_GPIO_WritePin(R2D_LED_GPIO_Port, R2D_LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TS_ON_LED_GPIO_Port, TS_ON_LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DRS_LED_GPIO_Port, DRS_LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(VC_LEDS_GPIO_Port, VC_LEDS_Pin, GPIO_PIN_RESET);

    return status;
}

/**
 * @brief       Dash service thread
 *
 * @param[in]   input   Pointer to dash instance
 */
void dash_thread_entry(ULONG input)
{
    dash_context_t *dash_ptr = (dash_context_t *)input;

    // see 'visible check' rule
    if (dash_ptr->config_ptr->vc_run_check)
    {
        run_visual_check(dash_ptr->config_ptr->vc_led_on_ticks);
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
void run_visual_check(uint32_t ticks)
{
    HAL_GPIO_WritePin(VC_LEDS_GPIO_Port, VC_LEDS_Pin, GPIO_PIN_SET);

    tx_thread_sleep(ticks);

    HAL_GPIO_WritePin(VC_LEDS_GPIO_Port, VC_LEDS_Pin, GPIO_PIN_RESET);
}

/**
 * @brief       Sets the TS on LED in toggle mode
 *
 * @param[in]   dash_ptr    Dash context
 * @param[in]   ticks       Number of ticks between toggles
 */
status_t dash_blink_ts_on_led(dash_context_t *dash_ptr, uint32_t ticks)
{
    UINT tx_status = tx_timer_deactivate(&dash_ptr->ts_on_toggle_timer);

    if (tx_status == TX_SUCCESS)
    {
        tx_status = tx_timer_change(&dash_ptr->ts_on_toggle_timer, ticks, ticks);
    }

    if (tx_status == TX_SUCCESS)
    {
        tx_status = tx_timer_activate(&dash_ptr->ts_on_toggle_timer);
    }

    return (tx_status == TX_SUCCESS) ? STATUS_OK : STATUS_ERROR;
}

/**
 * @brief       Toggles the TS on LED
 *
 * @param[in]   input   Dash context
 */
static void toggle_ts_on_callback(ULONG input)
{
    dash_context_t *dash_ptr = (dash_context_t *)input;
    UNUSED(dash_ptr);

    HAL_GPIO_TogglePin(TS_ON_LED_GPIO_Port, TS_ON_LED_Pin);
}

/**
 * @brief       Sets the state of the TS on LED
 *
 * @details     If the LED is currently being toggled, the toggling is disabled
 *
 * @param[in]   dash_ptr    Dash context
 * @param[in]   state       GPIO state
 */
status_t dash_set_ts_on_led_state(dash_context_t *dash_ptr, GPIO_PinState state)
{
    UINT is_toggling = 0;

    UINT tx_status = tx_timer_info_get(&dash_ptr->ts_on_toggle_timer,
                                       NULL,
                                       &is_toggling,
                                       NULL,
                                       NULL,
                                       NULL);

    if (is_toggling && (tx_status == TX_SUCCESS))
    {
        tx_status = tx_timer_deactivate(&dash_ptr->ts_on_toggle_timer);
    }

    HAL_GPIO_WritePin(TS_ON_LED_GPIO_Port, TS_ON_LED_Pin, state);

    return (tx_status == TX_SUCCESS) ? STATUS_OK : STATUS_ERROR;
}

/**
 * @brief   Waits until the TS on button is pressed and released
 *
 * @details The calling thread is suspended until this occurs
 */
status_t dash_wait_for_ts_on(dash_context_t *dash_ptr)
{
    input_context_t ts_on_input = {.port = TS_ON_BTN_GPIO_Port,
                                   .pin = TS_ON_BTN_Pin,
                                   .active_state = true,
                                   .last_state = false,
                                   .require_release = true,
                                   .active_start = 0,
                                   .required_ticks = dash_ptr->config_ptr->btn_active_ticks,
                                   .sample_ticks = dash_ptr->config_ptr->btn_sample_ticks};

    return input_wait(&ts_on_input);
}

/**
 * @brief   Waits until the TS on button is pressed and released
 *
 * @details The calling thread is suspended until this occurs
 */
status_t dash_wait_for_r2d(dash_context_t *dash_ptr)
{
    input_context_t r2d_input = {.port = R2D_BTN_GPIO_Port,
                                 .pin = R2D_BTN_Pin,
                                 .active_state = true,
                                 .last_state = false,
                                 .require_release = true,
                                 .active_start = 0,
                                 .required_ticks = dash_ptr->config_ptr->btn_active_ticks,
                                 .sample_ticks = dash_ptr->config_ptr->btn_sample_ticks};

    return input_wait(&r2d_input);
}

/**
 * @brief       Waits for an input to enter a state for a particular number
 *              of ticks and (optionally) exit that state
 *
 * @param[in]   input_ptr   Input context
 */
status_t input_wait(input_context_t *input_ptr)
{
    status_t status = STATUS_OK;
    bool done = false;

    while (!done && (status == STATUS_OK))
    {
        bool state = (HAL_GPIO_ReadPin(input_ptr->port, input_ptr->pin) == GPIO_PIN_SET);

        // rising edge (active, previously inactive)
        if (state == input_ptr->active_state && (input_ptr->last_state != input_ptr->active_state))
        {
            input_ptr->active_start = tx_time_get();
        }

        // held
        bool active_no_release_check = !input_ptr->require_release && (state == input_ptr->active_state);

        bool held_and_released = (state != input_ptr->active_state) && (input_ptr->last_state == input_ptr->active_state);

        if (active_no_release_check || held_and_released)
        {
            uint32_t active_ticks = tx_time_get() - input_ptr->active_start;

            if (active_ticks > input_ptr->required_ticks)
            {
                done = true;
            }
        }

        // update state and sleep
        input_ptr->last_state = state;

        status = (tx_thread_sleep(input_ptr->sample_ticks) == TX_SUCCESS)
                     ? STATUS_OK
                     : STATUS_ERROR;
    }

    return status;
}

/* Update button detection routine. Returns 0 if button hasn't been
   pressed, 1 if it has. After a press, the function returns 1 only once.
   This function should be called periodically and does not block */
bool input_update(input_context_t *input_ptr)
{
    bool pressed = false;
    bool state = (HAL_GPIO_ReadPin(input_ptr->port, input_ptr->pin) == GPIO_PIN_SET);

    // rising edge (active, previously inactive)
    if (state == input_ptr->active_state && (input_ptr->last_state != input_ptr->active_state))
    {
        input_ptr->active_start = tx_time_get();
    }

    // held
    bool active_no_release_check = !input_ptr->require_release && (state == input_ptr->active_state);

    bool held_and_released = (state != input_ptr->active_state) && (input_ptr->last_state == input_ptr->active_state);

    if (active_no_release_check || held_and_released)
    {
        uint32_t active_ticks = tx_time_get() - input_ptr->active_start;

        if (active_ticks > input_ptr->required_ticks)
        {
            pressed = true;
        }
    }

    // update state and sleep
    input_ptr->last_state = state;

    return pressed;
}

void dash_update_buttons(dash_context_t *dash_ptr)
{
    dash_ptr->tson_flag = input_update(&dash_ptr->tson);
    dash_ptr->r2d_flag = input_update(&dash_ptr->r2d);
}

void dash_clear_buttons(dash_context_t *dash_ptr)
{
    dash_ptr->tson_flag = false;
    dash_ptr->r2d_flag = false;
}

/**
 * @brief   Sets the state of the R2D LED
 *
 * @param[in]   dash_ptr    Dash context
 * @param[in]   state       State to set LED to
 */
status_t dash_set_r2d_led_state(dash_context_t *dash_ptr, GPIO_PinState state)
{
    UNUSED(dash_ptr);
    HAL_GPIO_WritePin(R2D_LED_GPIO_Port, R2D_LED_Pin, state);
    return STATUS_OK;
}
