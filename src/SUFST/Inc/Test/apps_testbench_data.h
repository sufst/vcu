/***************************************************************************
 * @file   apps_testbench_data.h
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-03-08
 * @brief  Data for APPS input test bench
 ***************************************************************************/

#include <stdint.h>

#include "config.h"

#ifndef APPS_TESTBENCH_DATA_H
#define APPS_TESTBENCH_DATA_H

#if RUN_APPS_TESTBENCH
extern const uint16_t standing_start_time_lookup[1001];
extern const uint16_t standing_start_apps_lookup[1001];
extern const uint16_t flying_lap_time_lookup[999];
extern const uint16_t flying_lap_apps_lookup[999];
#endif

#endif
