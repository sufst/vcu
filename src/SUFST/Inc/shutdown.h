/******************************************************************************
 * @file    shutdown.h
 * @author  Alexander Mills (Scaniox#7130 am9g22@soton.ac.uk)
 * @brief   shutdown interrupt causes the system to enter a shutdown thread and
 *          stay there until reset
 *****************************************************************************/
#ifndef SHUTDOWN_H
#define SHUTDOWN_H
#include "error_types.h"

typedef struct { 
    /**
     * @brief stores if the system is ready to drive or not, so the shutdown
     *        signal only takes affect when the car is RTD
     */
    Bool RTD;

    /**
     * @brief stores minor errors
     */

    /**
     * @brief stores the critical error that cause the system to shut down
     */

} shutdown_handle_t;



void shutdown_fault_registerer(shutdown_handle_t* sd_handle_t);

void Major_Error_Handler(shutdown_handle_t* sd_handle_t, 
                         Error_Types_e error_id);

void Minor_Error_Handler(shutdown_handle_t* sd_handle_t, 
                         Error_Types_e error_id);

#endif