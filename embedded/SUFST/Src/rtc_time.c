/***************************************************************************
 * @file   rtc_time.c
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2022-03-22
 * @brief  Real time clock time function implementation
 ***************************************************************************/

#include "rtc_time.h"

#include "rtc.h"

/**
 * @brief 	Initialise RTC time
 */
void rtc_time_init()
{
	RTC_TimeTypeDef time;
	memset(&time, 0x00, sizeof(time));
	HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN);
}

/**
 * @brief	Gets the current system time
 *
 * @return	Current system uptime in ms
 */
UINT rtc_time_get()
{
	RTC_TimeTypeDef time;
	HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, NULL, RTC_FORMAT_BIN);

	UINT time_ms = time.Seconds * 1000UL + (ULONG) ((time.SecondFraction - time.SubSeconds) * 999 / time.SecondFraction);
	return time_ms;
}

/**
 * @brief	 	Blocking delay using RTC
 *
 * @note		!!! Only use this in preference to tx_thread_sleep() if a delay is needed
 * 					before the RTOS scheduler is running
 *
 * @param[in]	delay	Delay in ms
 */
void rtc_delay(UINT delay)
{
	UINT start_time = rtc_time_get();
	UINT time_now = start_time;

	while (((time_now = rtc_time_get()) - start_time) < delay)
	{
		__asm__("NOP");
	}
}
