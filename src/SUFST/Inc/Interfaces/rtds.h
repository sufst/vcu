/***************************************************************************
 * @file   rtds.h
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief  Ready to drive speaker
 ***************************************************************************/

#ifndef RTDS_H
#define RTDS_H

#include <stdint.h>

#include "config.h"
#include "status.h"

status_t rtds_activate(const config_rtds_t* config_ptr);

#endif