#include "rtds.h"

#include <gpio.h>
#include <tx_api.h>

/**
 * @brief       Activates the RTDS and sleeps the calling thread for the
 *              duration of activation
 *
 * @param[in]   config_ptr  RTDS configuration
 */
status_t rtds_activate(const config_rtds_t *config_ptr, log_context_t *log_h)
{
    HAL_GPIO_WritePin(config_ptr->port, config_ptr->pin, GPIO_PIN_SET);
    LOG_INFO(log_h, "Waiting - RTDS\n");
    UINT tx_status = tx_thread_sleep(config_ptr->active_ticks);
    LOG_INFO(log_h, "Waited - RTDS\n");
    HAL_GPIO_WritePin(config_ptr->port, config_ptr->pin, GPIO_PIN_RESET);

    return (tx_status == TX_SUCCESS) ? STATUS_OK : STATUS_ERROR;
}