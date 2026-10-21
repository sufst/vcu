#include "rtds.h"

#include <FreeRTOS.h>
#include <task.h>

#include "io.h"

/**
 * @brief       Switches the ready-to-drive sound on or off
 *
 * @details     The control task times the sound (config active_ticks)
 *              itself, so the control loop keeps running while it plays
 *
 * @param[in]   config_ptr  RTDS configuration
 * @param[in]   on          True to sound the RTDS
 */
void rtds_set(const config_rtds_t *config_ptr, bool on)
{
    // unlike the other outputs, the RTDS is active high
    HAL_GPIO_WritePin(config_ptr->port, config_ptr->pin, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * @brief       Initialises non-blocking RTDS pulse state and ensures the
 *              RTDS output starts off
 *
 * @param[in]   pulse_ptr   RTDS pulse context
 * @param[in]   config_ptr  RTDS configuration
 */
void rtds_pulse_init(rtds_pulse_context_t *pulse_ptr, const config_rtds_t *config_ptr)
{
    pulse_ptr->config_ptr = config_ptr;
    pulse_ptr->active = false;
    pulse_ptr->pin_on = false;
    pulse_ptr->phase_start = 0;

    HAL_GPIO_WritePin(config_ptr->port, config_ptr->pin, GPIO_PIN_RESET);
}

/**
 * @brief       Advances the non-blocking RTDS pulse by one control-loop tick
 *
 * @param[in]   pulse_ptr   RTDS pulse context
 * @param[in]   enable      Whether the pulse should currently be running
 */
void rtds_pulse_tick(rtds_pulse_context_t *pulse_ptr, bool enable)
{
    const config_rtds_t *config_ptr = pulse_ptr->config_ptr;

    if (!enable)
    {
        if (pulse_ptr->pin_on)
        {
            HAL_GPIO_WritePin(config_ptr->port, config_ptr->pin, GPIO_PIN_RESET);
            pulse_ptr->pin_on = false;
        }
        pulse_ptr->active = false;
        return;
    }

    if (!pulse_ptr->active)
    {
        // rising edge: start the pulse immediately with the RTDS on
        pulse_ptr->active = true;
        pulse_ptr->pin_on = true;
        pulse_ptr->phase_start = xTaskGetTickCount();
        HAL_GPIO_WritePin(config_ptr->port, config_ptr->pin, GPIO_PIN_SET);
        return;
    }

    uint32_t phase_ticks = pulse_ptr->pin_on ? config_ptr->pulse_on_ticks :
                                               config_ptr->pulse_off_ticks;

    if (xTaskGetTickCount() >= pulse_ptr->phase_start + phase_ticks)
    {
        pulse_ptr->pin_on = !pulse_ptr->pin_on;
        HAL_GPIO_WritePin(config_ptr->port, config_ptr->pin,
                          pulse_ptr->pin_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
        pulse_ptr->phase_start = xTaskGetTickCount();
    }
}
