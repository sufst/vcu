#include "dash.h"

#include "io.h"
#include "rtos_utils.h"

/*
 * internal functions
 */
static void dash_thread_entry(void *arg);
static void run_visual_check(uint32_t ticks);
static void toggle_ts_on_callback(TimerHandle_t timer);
static bool input_update(input_context_t *input_ptr);

/**
 * @brief       Initialises the dash service
 *
 * @param[in]   dash_ptr            Dash context
 * @param[in]   config_ptr          Dash configuration
 */
status_t dash_init(dash_context_t *dash_ptr, const config_dash_t *config_ptr)
{
    status_t status = STATUS_OK;

    dash_ptr->config_ptr = config_ptr;
    dash_ptr->r2d_flag = false;
    dash_ptr->tson_flag = false;

    dash_ptr->r2d =
        (input_context_t){ .port = R2D_BTN_GPIO_Port,
                           .pin = R2D_BTN_Pin,
                           .active_state = true,
                           .last_state = false,
                           .require_release = true,
                           .active_start = 0,
                           .required_ticks = dash_ptr->config_ptr->btn_active_ticks };

    dash_ptr->tson =
        (input_context_t){ .port = TS_ON_BTN_GPIO_Port,
                           .pin = TS_ON_BTN_Pin,
                           .active_state = true,
                           .last_state = false,
                           .require_release = true,
                           .active_start = 0,
                           .required_ticks = dash_ptr->config_ptr->btn_active_ticks };

    // create the thread
    status = rtos_task_create(&config_ptr->thread, dash_thread_entry, dash_ptr,
                              &dash_ptr->thread);

    // create timer for toggling TS on LED (not started until needed)
    dash_ptr->ts_on_toggle_timer =
        xTimerCreateStatic("TS on LED", portMAX_DELAY, pdTRUE, dash_ptr,
                           toggle_ts_on_callback, &dash_ptr->ts_on_toggle_timer_cb);

    if (dash_ptr->ts_on_toggle_timer == NULL)
    {
        status = STATUS_ERROR;
    }

    // CubeMX will auto-generate code which sets initial pin states
    // however for ease of editing / readability it's done again here

    /* LEDs are active-low; set outputs to 'high' (external off)
        to ensure LEDs are off at init. */
    VCU_Output_High(R2D_LED_GPIO_Port, R2D_LED_Pin);
    VCU_Output_High(TS_ON_LED_GPIO_Port, TS_ON_LED_Pin);
    //  VCU_Output_High(DRS_LED_GPIO_Port, DRS_LED_Pin);
    VCU_Output_High(VC_LEDS_GPIO_Port, VC_LEDS_Pin);

    return status;
}

/**
 * @brief       Dash service thread
 *
 * @details     Runs the visual check once, then deletes itself
 *
 * @param[in]   arg     Pointer to dash instance
 */
static void dash_thread_entry(void *arg)
{
    const dash_context_t *dash_ptr = (dash_context_t *)arg;

    // see 'visible check' rule
    if (dash_ptr->config_ptr->vc_run_check)
    {
        run_visual_check(dash_ptr->config_ptr->vc_led_on_ticks);
    }

    // FreeRTOS tasks must not return from their entry function
    vTaskDelete(NULL);
}

/**
 * @brief       Runs the 'visual check'
 *
 * @details     Must be called from a thread
 *
 * @param[in]   ticks           Number of ticks to keep all LEDs on
 */
static void run_visual_check(uint32_t ticks)
{
    VCU_Output_Low(VC_LEDS_GPIO_Port, VC_LEDS_Pin);

    vTaskDelay(ticks);

    VCU_Output_High(VC_LEDS_GPIO_Port, VC_LEDS_Pin);
}

/**
 * @brief       Sets the TS on LED in toggle mode
 *
 * @details     This is called on every control loop tick while in a fault
 *              state, so if the LED is already blinking at the requested
 *              rate the timer is left running. (Restarting it every tick
 *              would stop it from ever expiring, so the LED would never
 *              toggle.)
 *
 * @param[in]   dash_ptr    Dash context
 * @param[in]   ticks       Number of ticks between toggles
 */
status_t dash_blink_ts_on_led(dash_context_t *dash_ptr, uint32_t ticks)
{
    TimerHandle_t timer = dash_ptr->ts_on_toggle_timer;

    if (xTimerIsTimerActive(timer) && (xTimerGetPeriod(timer) == ticks))
    {
        return STATUS_OK;
    }

    // changing the period also (re)starts the timer
    BaseType_t rc = xTimerChangePeriod(timer, ticks, 0);

    return (rc == pdPASS) ? STATUS_OK : STATUS_ERROR;
}

/**
 * @brief       Toggles the TS on LED
 *
 * @param[in]   timer   Timer handle
 */
static void toggle_ts_on_callback(TimerHandle_t timer)
{
    UNUSED(timer);

    VCU_Output_Toggle(TS_ON_LED_GPIO_Port, TS_ON_LED_Pin);
}

/* Update button detection routine. Returns 0 if button hasn't been
   pressed, 1 if it has. After a press, the function returns 1 only once.
   This function should be called periodically and does not block */
static bool input_update(input_context_t *input_ptr)
{
    bool pressed = false;
    bool state = VCU_Input_Read(input_ptr->port, input_ptr->pin);

    // rising edge (active, previously inactive)
    if (state == input_ptr->active_state && (input_ptr->last_state != input_ptr->active_state))
    {
        input_ptr->active_start = xTaskGetTickCount();
    }

    // held
    bool active_no_release_check =
        !input_ptr->require_release && (state == input_ptr->active_state);

    bool held_and_released = (state != input_ptr->active_state) &&
        (input_ptr->last_state == input_ptr->active_state);

    if (active_no_release_check || held_and_released)
    {
        TickType_t active_ticks = xTaskGetTickCount() - input_ptr->active_start;

        if (active_ticks > input_ptr->required_ticks)
        {
            pressed = true;
        }
    }

    // update state
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
    return VCU_Output_Write(R2D_LED_GPIO_Port, R2D_LED_Pin, state);
}
