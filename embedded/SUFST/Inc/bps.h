/***************************************************************************
 * @file   bps.h
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-07-02
 * @brief  BPS input function prototypes
 ***************************************************************************/

#ifndef BPS_H
#define BPS_H

#include <stdint.h>

void bps_init();
uint32_t bps_read();

#endif