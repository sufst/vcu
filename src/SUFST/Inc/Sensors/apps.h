/***************************************************************************
 * @file   apps.h
 * @author Tim Brewis (@t-bre)
 * @brief  Accelerator pedal position sensor
 ***************************************************************************/

#ifndef APPS_H
#define APPS_H

#include <stdint.h>

void apps_init();
uint32_t apps_read();

#endif