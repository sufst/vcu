/*****************************************************************************
 * @file   heartbeat.c,
 * @author George Peppard (@inventor02, gjp1g21@soton.ac.uk)
 * @brief  Heartbeat service implementation (glorified LED blinker)
 ****************************************************************************/

#include "heartbeat.h"

/*
 * internal function prototypes
 */
static void heartbeat_thread_entry(ULONG input);

/**
 * @brief       Initialise heartbeat service
 *
 * @param[in]   heartbeat_h     Heartbeat handle
 * @param[in]   stack_pool_ptr  Application memory pool
 * @param[in]   config_ptr      Configuration
 */
status_t heartbeat_init(heartbeat_context_t* heartbeat_h,
                        TX_BYTE_POOL* stack_pool_ptr,
                        const config_heartbeat_t* config_ptr)
{
    heartbeat_h->config_ptr = config_ptr;

    // create service thread
    void* stack_ptr = NULL;
    UINT tx_status = tx_byte_allocate(stack_pool_ptr,
                                      &stack_ptr,
                                      config_ptr->thread.stack_size,
                                      TX_NO_WAIT);

    if (tx_status == TX_SUCCESS)
    {
        tx_status = tx_thread_create(&heartbeat_h->thread,
                                     (CHAR*) config_ptr->thread.name,
                                     heartbeat_thread_entry,
                                     (ULONG) heartbeat_h,
                                     stack_ptr,
                                     config_ptr->thread.stack_size,
                                     config_ptr->thread.priority,
                                     config_ptr->thread.priority,
                                     TX_NO_TIME_SLICE,
                                     TX_AUTO_START);
    }

    return (tx_status == TX_SUCCESS) ? STATUS_OK : STATUS_ERROR;
}

/**
 * @brief Heartbeat service thread
 * @details Flashes an LED at a defined period
 *
 * @param input heartbeat handle
 */
static void heartbeat_thread_entry(ULONG input)
{
    heartbeat_context_t* heartbeat_h = (heartbeat_context_t*) input;

    // initialise LED
    HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);

    // main loop
    while (1)
    {
        // toggle LED
        HAL_GPIO_TogglePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin);

        // wait
        tx_thread_sleep(heartbeat_h->config_ptr->blink_period_ticks);
    }
}