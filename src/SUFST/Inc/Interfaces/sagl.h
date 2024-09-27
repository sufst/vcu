/***************************************************************************
 * @file   sagl.c
 * @author Dmytro Avdieienko (@Avdieienko)
 * @brief  Steering Wheel Angle Sensor
 ***************************************************************************/

#ifndef SAGL_H
#define SAGL_H

#include <stdbool.h>
#include <stdint.h>

#include "config.h"
#include "scs.h"
#include "status.h"

/**
 * @brief   SAGL context
 */
typedef struct
{
    scs_t signal;
    const config_sagl_t* config_ptr;
} sagl_context_t;

/*
 * public functions
 */
status_t sagl_init(sagl_context_t* sagl_ptr, const config_sagl_t* config_ptr);
status_t sagl_read(sagl_context_t* sagl_ptr, uint16_t* reading_ptr);

#endif