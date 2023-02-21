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

#define MAX_MINOR_ERRORS 5


typedef struct { 
    /**
     * @brief stores if the system is ready to drive or not, so the shutdown
     *        signal only takes affect when the car is RTD
     */
    bool shutdown_ISR_enable;

    /**
     * @brief stores minor errors
     */
    Error_Types_e minor_errors[MAX_MINOR_ERRORS];

    /**
     * @brief the number of minor errors
     */
    uint32_t minor_error_count;


    /**
     * @brief stores the critical error that cause the system to shut down
     */
    Error_Types_e major_error;

} shutdown_handle_t;



void shutdown_fault_registerer(shutdown_handle_t* sd_handle);

void major_Error_Handler(shutdown_handle_t* sd_handle_t, 
                         Error_Types_e error_id);

void minor_Error_Handler(shutdown_handle_t* sd_handle, 
                         Error_Types_e error_id);

void shutdown_handler(shutdown_handle_t* sd_handle);

void shutdown_post_RTD(shutdown_handle_t* sd_handle);

#endif