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


void shutdown_init(shutdown_handle_t* sd_handle,
                    void (*callback)(ULONG),
                    ULONG callback_arg)
{
    sd_handle->shutdown_ISR_enable = false;
    sd_handle->minor_error_count = 0;

    sd_handle->callback = callback;
    sd_handle->callback_arg = callback_arg;

}



 /**
  * @brief checks RTD status before registering a critical error
  */
void shutdown_fault_registerer(shutdown_handle_t* sd_handle)
{
    if (sd_handle->shutdown_ISR_enable)
    {
        major_Error_Handler(sd_handle, SD_Shutdown_ERROR);
    }
}


/**
 * @brief is used for registering a major fault
 */
void major_Error_Handler(shutdown_handle_t* sd_handle, SD_Error_Types Error)
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
void minor_Error_Handler(shutdown_handle_t* sd_handle, SD_Error_Types Error)
{
    if (sd_handle->minor_error_count < SHUTDOWN_MAX_MINOR_ERRORS)
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
 * gets the vcu to:
 * dissables sending messages to TS
 * dissabled recieving from driver inputs
 * starts broadcasting error state
 */
void shutdown_handler(shutdown_handle_t* sd_handle)
{
    if(sd_handle->callback != NULL)
    {
        sd_handle->callback(sd_handle->callback_arg);
    }
}


/**
 * @brief notifies the shutdown system of RTD (enables shutdown interrupt)
 * 
 */
void shutdown_post_RTD(shutdown_handle_t* sd_handle)
{
  sd_handle->shutdown_ISR_enable = true;
}