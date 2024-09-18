/*****************************************************************************
 * @file   error_handler.c,
 * @author Dmytro Avdieienko (@Avdieienko, da3e22@soton.ac.uk)
 * @brief  Custom Error Handler service implementation
 ****************************************************************************/

#include "error_handler.h"

/*
 * internal function prototypes
 */
static void error_handler_thread_entry(ULONG input);

/**
 * @brief       Initialise error handler service
 *
 * @param[in]   error_handler_h     Error Handler handle
 * @param[in]   stack_pool_ptr  Application memory pool
 * @param[in]   config_ptr      Configuration
 */
status_t error_handler_init(error_handler_context_t* error_handler_h,
                        TX_BYTE_POOL* stack_pool_ptr,
                        const config_error_handler_t* config_ptr)
{
    error_handler_h->config_ptr = config_ptr;

    // create service thread
    void* stack_ptr = NULL;
    UINT tx_status = tx_byte_allocate(stack_pool_ptr,
                                      &stack_ptr,
                                      config_ptr->thread.stack_size,
                                      TX_NO_WAIT);

    if (tx_status == TX_SUCCESS)
    {
        tx_status = tx_thread_create(&error_handler_h->thread,
                                     (CHAR*) config_ptr->thread.name,
                                     error_handler_thread_entry,
                                     (ULONG) error_handler_h,
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
 * @brief Error Handler service thread
 * @details Flashes an LED at a defined period
 *
 * @param input error handler handle
 */
static void error_handler_thread_entry(ULONG input)
{
    error_handler_context_t* error_handler_h = (error_handler_context_t*) input;
}