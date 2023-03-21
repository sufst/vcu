/******************************************************************************
 * @file    error_types.h
 * @author  Alexander Mills (Scaniox#7130 am9g22@soton.ac.uk)
 * @brief   error type identifiers to be used when calling error handlers to
 *          identify what system caused the fault
 *****************************************************************************/
#ifndef ERROR_TYPES_DEF
#define ERROR_TYPES_DEF

typedef enum {
    SD_APPS_ERROR, SD_BPS_ERROR, SD_SCS_ERROR,
    SD_TS_ERROR, SD_Shutdown_ERROR, SD_GOOD
} SD_Error_Types;

#endif