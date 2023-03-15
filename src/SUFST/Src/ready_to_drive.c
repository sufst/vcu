/***************************************************************************
 * @file   ready_to_drive.c
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-03-10
 * @brief  Ready to drive logic implementation
 ***************************************************************************/

#include "ready_to_drive.h"

#include <stdbool.h>

#include "bps.h"
#include "config.h"
#include "gpio.h"
#include "tx_api.h"

static void start_speaker_sound(rtd_context_t* rtd_ptr);
static void spkr_timer_expiry_callback(ULONG input);

/**
 * @brief   Initialises RTD context
 *
 * @param[in]   rtd_ptr             RTD instance
 * @param[in]   spkr_gpio_port      GPIO port for speaker
 * @param[in]   spkr_gpio_pin       GPIO pin for speaker
 * @param[in]   spkr_on_time        Time in ticks for which speaker should sound
 * @param[in]   enable_bps_check    Only proceed to RTD if BPS fully pressed
 */
void rtd_init(rtd_context_t* rtd_ptr,
              GPIO_TypeDef* spkr_gpio_port,
              uint32_t spkr_gpio_pin,
              uint32_t spkr_on_time,
              bool enable_bps_check)
{
    rtd_ptr->spkr_gpio_port = spkr_gpio_port;
    rtd_ptr->spkr_gpio_pin = spkr_gpio_pin;

    atomic_store(&rtd_ptr->ready, false);

    // semaphore
    UINT tx_status;

    tx_status = tx_semaphore_create(&rtd_ptr->sem, "RTD Semaphore", 0);

    // speaker timer
    if (tx_status == TX_SUCCESS)
    {
        tx_status = tx_timer_create(&rtd_ptr->speaker_timer,
                                    "RTD Speaker Timer",
                                    spkr_timer_expiry_callback,
                                    (ULONG) rtd_ptr,
                                    spkr_on_time,
                                    0,
                                    TX_NO_ACTIVATE);
    }

    (void) tx_status;
}

/**
 * @brief       Suspends the calling thread until RTD signal received
 *
 * @param[in]   rtd_ptr     RTD context
 */
void rtd_wait(rtd_context_t* rtd_ptr)
{
    tx_semaphore_get(&rtd_ptr->sem, TX_WAIT_FOREVER);
}

/**
 * @brief       Checks if ready to drive is complete
 *
 * @param[in]   rtd_ptr     RTD context
 */
bool rtd_is_ready(rtd_context_t* rtd_ptr)
{
    return atomic_load(&rtd_ptr->ready);
}

/**
 * @brief       Handles interrupt
 *
 * @param[in]   rtd_ptr
 */
void rtd_handle_int(rtd_context_t* rtd_ptr)
{
    bool is_rtd = false;

    if (!rtd_ptr->enable_bps_check)
    {
        is_rtd = true;
    }
    else
    {
        is_rtd = bps_fully_pressed();
    }

    if (is_rtd && !rtd_is_ready(rtd_ptr)) // debounce
    {
        atomic_store(&rtd_ptr->ready, true);
        start_speaker_sound(rtd_ptr);
    }
}

/**
 * @brief       Turns on the RTD speaker and starts the timer
 *
 * @note        If this is called twice (e.g. due to RTD input debouncing
 *              issue), there will be a `TX_ACTIVATE_ERROR` and the speaker
 * sound will NOT be started. `TX_TIMER_ERROR` should never happen, so it is
 * checked.
 *
 * @param[in]   rtd_ptr     RTD context
 */
static void start_speaker_sound(rtd_context_t* rtd_ptr)
{
    HAL_GPIO_WritePin(rtd_ptr->spkr_gpio_port,
                      rtd_ptr->spkr_gpio_pin,
                      GPIO_PIN_SET);

    UINT tx_status = tx_timer_activate(&rtd_ptr->speaker_timer);

    if (tx_status == TX_TIMER_ERROR)
    {
        // just in case - don't want the speaker to stay on for ever!
        HAL_GPIO_WritePin(rtd_ptr->spkr_gpio_port,
                          rtd_ptr->spkr_gpio_pin,
                          GPIO_PIN_RESET);

        Error_Handler();
    }
}

/**
 * @brief       Called on expiry of the speaker timer
 *
 * @details     All threads which have called rtd_wait() will be resumed
 *
 * @param[in]   input   RTD context
 */
static void spkr_timer_expiry_callback(ULONG input)
{
    rtd_context_t* rtd_ptr = (rtd_context_t*) input;

    HAL_GPIO_WritePin(rtd_ptr->spkr_gpio_port,
                      rtd_ptr->spkr_gpio_pin,
                      GPIO_PIN_RESET);

    // notify ALL suspended threads
    ULONG sem_suspended_count;

    UINT tx_status = tx_semaphore_info_get(&rtd_ptr->sem,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &sem_suspended_count,
                                           NULL);

    if (tx_status == TX_SUCCESS)
    {
        for (uint32_t i = 0; i < sem_suspended_count; i++)
        {
            tx_status = tx_semaphore_put(&rtd_ptr->sem);

            if (tx_status != TX_SUCCESS)
            {
                break;
            }
        }
    }
}
