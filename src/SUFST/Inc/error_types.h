/******************************************************************************
 * @file    error_types.h
 * @author  Alexander Mills (Scaniox#7130 am9g22@soton.ac.uk)
 * @brief   error type identifiers to be used when calling error handlers to
 *          identify what system caused the fault
 *****************************************************************************/
#ifndef ERROR_TYPES_DEF
#define ERROR_TYPES_DEF

typedef enum {
    APPS_Error, BPS_Error, SCS_Error, TS_Error, Shutdown_Error, Good
} Error_Types_e;

#endif