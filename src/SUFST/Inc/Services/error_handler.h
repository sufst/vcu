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
#include "canbc.h"
#include "log.h"
#include "ctrl.h"

// Error criticality codes
#define ERROR_CODE_CRITICAL 0x01
#define ERROR_CODE_NON_CRITICAL 0x02
// Error codes based on service
#define ERROR_CODE_RTCAN_S 0x04
#define ERROR_CODE_RTCAN_C 0x08

typedef struct
{
    TX_THREAD thread;

    canbc_context_t* canbc_ptr;
    ctrl_context_t* ctrl_ptr;

    const config_error_handler_t* config_ptr;
} error_handler_context_t;

status_t error_handler_init(error_handler_context_t* error_handler_ptr,
                        TX_BYTE_POOL* stack_pool_ptr,
                        canbc_context_t* canbc_ptr,
                        ctrl_context_t* ctrl_ptr,
                        log_context_t* log_ptr,
                        const config_error_handler_t* config_ptr);

void handle_error(error_handler_context_t* error_handler_ptr,
                      const uint32_t error_code);

status_t shutdown_system(ctrl_context_t* ctrl_ptr);

status_t broadcast_error(error_handler_context_t* error_handler_ptr,
                         const uint32_t error_code);

#endif