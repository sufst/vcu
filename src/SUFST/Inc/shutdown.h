/******************************************************************************
 * @file    shutdown.h
 * @author  Alexander Mills (Scaniox#7130 am9g22@soton.ac.uk)
 * @brief   shutdown interrupt causes the system to enter a shutdown thread and
 *          stay there until reset
 *****************************************************************************/
#ifndef SHUTDOWN_H
#define SHUTDOWN_H
#include "error_types.h"
#include <stdbool.h>
#include <stdint.h>
#include <tx_api.h>

#define SHUTDOWN_MAX_MINOR_ERRORS 5


typedef struct { 
    
    void (*callback)(ULONG);
    ULONG callback_arg;

    /**
     * @brief stores if the system is ready to drive or not, so the shutdown
     *        signal only takes affect when the car is RTD
     */
    bool shutdown_ISR_enable;

    /**
     * @brief stores minor errors
     */
    SD_Error_Types minor_errors[SHUTDOWN_MAX_MINOR_ERRORS];

    /**
     * @brief the number of minor errors
     */
    uint32_t minor_error_count;

    /**
     * @brief stores the critical error that cause the system to shut down
     */
    SD_Error_Types major_error;

} shutdown_handle_t;


void shutdown_init(shutdown_handle_t* sd_handle,
                    void (*callback)(ULONG),
                     ULONG callback_arg);

void shutdown_fault_registerer(shutdown_handle_t* sd_handle);

void major_Error_Handler(shutdown_handle_t* sd_handle_t, 
                         SD_Error_Types error_id);

void minor_Error_Handler(shutdown_handle_t* sd_handle, 
                         SD_Error_Types error_id);

void shutdown_handler(shutdown_handle_t* sd_handle);

void shutdown_post_RTD(shutdown_handle_t* sd_handle);

#endif