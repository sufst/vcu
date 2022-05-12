/***************************************************************************
 * @file   rtc_time.h
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2022-03-22
 * @brief  Real time clock time function prototypes
 ***************************************************************************/

#ifndef RTC_TIME_H
#define RTC_TIME_H

#include "tx_api.h"

void rtc_time_init();
UINT rtc_time_get();
void rtc_delay(UINT delay);

#endif
