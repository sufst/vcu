/*****************************************************************************
 * @file   error_handler.h,
 * @author Dmytro Avdieienko (@Avdieienko, da3e22@soton.ac.uk)
 * @brief  Custom Error Handler service prototype
 ****************************************************************************/

#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <tx_api.h>

#include "config.h"
#include "status.h"

typedef struct
{
    TX_THREAD thread;
    const config_error_handler_t* config_ptr;
} error_handler_context_t;

status_t error_handler_init(error_handler_context_t* error_handler_h,
                        TX_BYTE_POOL* stack_pool_ptr,
                        const config_error_handler_t* config_ptr);

#endif