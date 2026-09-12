#include "rtds.h"

#include <tx_api.h>

#include "io.h"

/**
 * @brief       Activates the RTDS and sleeps the calling thread for the
 *              duration of activation
 *
 * @param[in]   config_ptr  RTDS configuration
 */
status_t rtds_activate(const config_rtds_t *config_ptr)
{

    // unlinke other outouts, the RTDS is active high.
    HAL_GPIO_WritePin(config_ptr->port, config_ptr->pin, GPIO_PIN_SET);
    LOG_INFO("Waiting - RTDS\n");
    UINT tx_status = tx_thread_sleep(config_ptr->active_ticks);
    LOG_INFO("Waited - RTDS\n");
    HAL_GPIO_WritePin(config_ptr->port, config_ptr->pin, GPIO_PIN_RESET);

    return (tx_status == TX_SUCCESS) ? STATUS_OK : STATUS_ERROR;
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
        pulse_ptr->phase_start = tx_time_get();
        HAL_GPIO_WritePin(config_ptr->port, config_ptr->pin, GPIO_PIN_SET);
        return;
    }

    uint32_t phase_ticks = pulse_ptr->pin_on ? config_ptr->pulse_on_ticks :
                                               config_ptr->pulse_off_ticks;

    if (tx_time_get() >= pulse_ptr->phase_start + phase_ticks)
    {
        pulse_ptr->pin_on = !pulse_ptr->pin_on;
        HAL_GPIO_WritePin(config_ptr->port, config_ptr->pin,
                          pulse_ptr->pin_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
        pulse_ptr->phase_start = tx_time_get();
    }
}
