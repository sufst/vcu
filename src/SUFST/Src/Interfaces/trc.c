#include "trc.h"

#include <stdbool.h>
#include <tx_api.h>

/**
 * @brief       Sets the state of the TS on connection to the TRC
 *
 * @details     The TS becomes ready when TS on is active and the TRC is not
 *              in a fault state. The TRC fault state is controlled by the
 *              inverter fault output.
 *
 * @param[in]   state   TS on pin state
 */
status_t trc_set_ts_on(GPIO_PinState state)
{
    HAL_GPIO_WritePin(TS_ON_GPIO_Port, TS_ON_Pin, state);
    return STATUS_OK;
}

bool trc_ready(void)
{
    return HAL_GPIO_ReadPin(TS_READY_GPIO_Port, TS_READY_Pin);
}

/**
 * @brief       Waits for the TRC to enter the ready state or a timeout
 *
 * @details     Blocks the calling thread until done, periodically waking
 *              the thread to read the input. If timed out, returns STATUS_ERROR
 *
 * @param[in]   poll_ticks  Ticks between checking the input state
 * @param[in]   timeout     Activation timeout
 */
status_t trc_wait_for_ready(uint32_t poll_ticks, uint32_t timeout)
{
    uint32_t time_start = tx_time_get();
    status_t status = STATUS_OK;
    bool done = false;

    while (!done && (status == STATUS_OK))
    {
        status = (tx_thread_sleep(poll_ticks) == TX_SUCCESS) ? STATUS_OK
                                                             : STATUS_ERROR;

        bool ready_high = (HAL_GPIO_ReadPin(TS_READY_GPIO_Port, TS_READY_Pin) == GPIO_PIN_SET);

        if (ready_high)
        {
            done = true;
        }
        else if ((tx_time_get() - time_start) >= timeout)
        {
            status = STATUS_ERROR; // timeout is an error
        }
    }

    return status;
}
