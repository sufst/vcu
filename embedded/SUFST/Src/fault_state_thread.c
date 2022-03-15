/***************************************************************************
 * @file   fault_state_thread.c
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2022-03-15
 * @brief  Fault state thread
 ***************************************************************************/

#include "fault_state_thread.h"
#include "config.h"

#include "gpio.h"

/**
 * @brief Thread for fault state
 */
TX_THREAD fault_state_thread;

/**
 * @brief 		Fault state thread entry function
 *
 * @param[in]	thread_input	Unused input
 */
void fault_state_thread_entry(ULONG thread_input)
{
	__asm__("NOP");



}
