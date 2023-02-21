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
void shutdown_fault_registerer(shutdown_handle_t* sd_handle)
{
    if(sd_handle->RTD) 
        Major_Error_Handler(sd_handle, Shutdown_Error);
}


/**
 * @brief is used for registering a major fault
 */
void Major_Error_Handler(shutdown_handle_t* sd_handle, Error_Types_e Error)
{
  // don't overide previous major errors
  if (!(sd_handle->major_error)) 
  {
    sd_handle->major_error = Error;
    
    shutdown_handler(sd_handle);
  }
}


/**
 * @brief is used for registering a minor fault
 * when called, the error type (see error_types.h) for the module that calls it 
 * should be provided
 */
void Minor_Error_Handler(shutdown_handle_t* sd_handle, Error_Types_e Error)
{
    if(sd_handle->minor_error_count < MAX_MINOR_ERRORS)
    {
        sd_handle->minor_errors[sd_handle->minor_error_count] = Error;
        sd_handle->minor_error_count++;
    }
    else
    {
        shutdown_handler(sd_handle);
    }
}


/**
 * @brief called if the system needs to shutdown:
 * dissables sending messages to TS
 * dissabled recieving from driver inputs
 * starts broadcasting error state
 */
void shutdown_handler(shutdown_handle_t* sd_handle)
{
  // stop TS control thread

  // stop driver control thread

  // enable broadcasting of errors
}