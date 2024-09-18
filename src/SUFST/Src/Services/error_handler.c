/*****************************************************************************
 * @file   error_handler.c,
 * @author Dmytro Avdieienko (@Avdieienko, da3e22@soton.ac.uk)
 * @brief  Custom Error Handler service implementation
 ****************************************************************************/

#include "error_handler.h"
#include "trc.h"

/*
 * internal function prototypes
 */
static void error_handler_thread_entry(ULONG input);

/**
 * @brief       Initialise error handler service
 *
 * @param[in]   error_handler_ptr     Error Handler pointer
 * @param[in]   stack_pool_ptr  Application memory pool
 * @param[in]   config_ptr      Configuration
 */
status_t error_handler_init(error_handler_context_t* error_handler_ptr,
                        TX_BYTE_POOL* stack_pool_ptr,
                        canbc_context_t* canbc_ptr,
                        ctrl_context_t* ctrl_ptr,
                        log_context_t* log_ptr,
                        const config_error_handler_t* config_ptr)
{
    error_handler_ptr->config_ptr = config_ptr;
    error_handler_ptr->canbc_ptr = canbc_ptr;
    error_handler_ptr->ctrl_ptr = ctrl_ptr;
    error_handler_ptr->log_ptr = log_ptr;

    // create service thread
    void* stack_ptr = NULL;
    UINT tx_status = tx_byte_allocate(stack_pool_ptr,
                                      &stack_ptr,
                                      config_ptr->thread.stack_size,
                                      TX_NO_WAIT);

    if (tx_status == TX_SUCCESS)
    {
        tx_status = tx_thread_create(&error_handler_ptr->thread,
                                     (CHAR*) config_ptr->thread.name,
                                     error_handler_thread_entry,
                                     (ULONG) error_handler_ptr,
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
 *
 * @param input error handler context
 */
static void error_handler_thread_entry(ULONG input)
{
    error_handler_context_t* error_handler_ptr = (error_handler_context_t*) input;
}

/**
 * @brief       Handle an error
 *
 * @param[in]   error_handler_ptr     Error Handler pointer
 * @param[in]   error_code          Error code
 */
status_t handle_error(error_handler_context_t* error_handler_ptr,
                      const uint32_t error_code)
{
  // TODO: split error for critical and non-critical
  // if critical, broadcast error and shutdown system
  // if non-critical, log error and broadcast error
  // if multiple non-critical errors - shutdown system
  return STATUS_OK;
}

/**
 * @brief       Safely shutdown the system in case of critical error
 *
 * @param[in]   ctrl_ptr     Control context
 */
status_t shutdown_system(ctrl_context_t* ctrl_ptr)
{
  dash_context_t* dash_ptr = ctrl_ptr->dash_ptr;
  const config_ctrl_t* config_ptr = ctrl_ptr->config_ptr;

  ctrl_ptr->inverter_pwr = false;
  ctrl_ptr->pump_pwr = false;
  ctrl_ptr->fan_pwr = false;

  trc_set_ts_on(GPIO_PIN_RESET);
  dash_blink_ts_on_led(dash_ptr, config_ptr->error_led_toggle_ticks);
  ctrl_update_canbc_states(ctrl_ptr);
  return STATUS_OK;
}

/**
 * @brief       Broadcast an error to CANBC
 *
 * @param[in]   error_handler_ptr     Error Handler pointer
 * @param[in]   error_code          Error code
 */
status_t broadcast_error(error_handler_context_t* error_handler_ptr,
                         const uint32_t error_code)
{
  canbc_states_t* states = canbc_lock_state(error_handler_ptr->canbc_ptr, TX_NO_WAIT);
  if(states != NULL)
  {
    // TODO: update broadcast states with error
    // Do a switch case on error_code and update the vcu error states accordingly to service
    canbc_unlock_state(error_handler_ptr->canbc_ptr);
  }
  return STATUS_OK;
}