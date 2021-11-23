/***************************************************************************
 * @file   led_thread.c
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2021-11-09
 * @brief  LED thread
 ***************************************************************************/

#include "led_thread.h"

TX_THREAD led_thread;

void led_thread_entry(ULONG thread_input)
{
	(VOID) thread_input;

	while (1)
	{

	}
}
