/***************************************************************************
 * @file   apps.h
 * @author Tim Brewis (@t-bre)
 * @brief  Accelerator pedal position sensor
 ***************************************************************************/

#ifndef APPS_H
#define APPS_H

#include <stdint.h>

#include "config.h"
#include "scs.h"
#include "status.h"
#include "log.h"

/**
 * @brief   APPS context
 */
typedef struct
{
     scs_t apps_1_signal;             // SCS instance for first APPS signal
     scs_t apps_2_signal;             // SCS instance for second APPS signal
     uint32_t scs_error;              // SCS error code (see scs.h)
     const config_apps_t* config_ptr; // configuration
     log_context_t *log_ptr;
} apps_context_t;

/*
 * public functions
 */
status_t apps_init(apps_context_t* apps_ptr, log_context_t *log_ptr,
		   const config_apps_t* config_ptr);
status_t apps_read(apps_context_t* apps_ptr, uint16_t* reading_ptr);
bool apps_check_plausibility(apps_context_t* apps_ptr);

#endif
