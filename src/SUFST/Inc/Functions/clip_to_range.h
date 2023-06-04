/******************************************************************************
 * @file    clip_to_range.h
 * @author  Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief   Function which a uint16_t to a certain range
 *****************************************************************************/

#ifndef CLIP_TO_RANGE_H
#define CLIP_TO_RANGE_H

#include <stdint.h>

uint16_t clip_to_range(uint16_t value, uint16_t min, uint16_t max);

#endif