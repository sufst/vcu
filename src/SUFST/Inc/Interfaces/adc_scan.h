/***************************************************************************
 * @file   adc_scan.h
 * @brief  Free-running scan+DMA ADC sampling, with rolling-average readout
 ***************************************************************************/

#ifndef ADC_SCAN_H
#define ADC_SCAN_H

#include <adc.h>
#include <stdbool.h>
#include <stdint.h>

#include "status.h"

/*
 * ADC1 and ADC2 are configured (see adc.c) to continuously scan their
 * regular channel sequence into a circular DMA buffer. adc_scan_init()
 * starts that free-running scan; adc_scan_get_average() reads back a
 * rolling average (~1ms window) for a given channel's slot in the scan
 * sequence (0-based, i.e. rank - 1).
 */

status_t adc_scan_init(void);
uint16_t adc_scan_get_average(const ADC_HandleTypeDef *hadc, uint8_t slot);
bool adc_scan_is_ready(const ADC_HandleTypeDef *hadc);

#endif // ADC_SCAN_H
