/******************************************************************************
 * @file    shutdown.c
 * @author  Alexander Mills (Scaniox#7130 am9g22@soton.ac.uk)
 * @brief   shutdown interrupt causes the system to enter a shutdown thread
 *****************************************************************************/

#include "shutdown.h"

#include <stdint.h>

#include "tx_api.h"

#include "gpio.h"

#include <inttypes.h>

 /**
  * @brief checks RTD status before registering a critical error
  */
void shutdown_fault_registerer(shutdown_handle_t* sd_handle_t)
{
    if(sd_handle_t->RTD) 
      Major_Error_Handler(sd_handle_t, Shutdown_Error);
}


/**
 * @brief is used for registering a major fault
 */
void Major_Error_Handler(shutdown_handle_t* sd_handle_t, Error_Types_e Error) {
}


/**
 * @brief is used for registering a minor fault
 * when called, the error type (see error_types.h) for the module that calls it 
 * should be provided
 */
void Minor_Error_Handler(shutdown_handle_t* sd_handle_t, Error_Types_e Error) {
}