/***************************************************************************
 * @file   fault_state_thread.c
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-03-15
 * @brief  Fault state thread implementation
 ***************************************************************************/

#include "fault_state_thread.h"
#include "config.h"

#include "can_tx_thread.h"
#include "can_rx_thread.h"
#include "control_thread.h"
#include "sensor_thread.h"
#include "gpio.h"

#define FAULT_STATE_THREAD_STACK_SIZE				512
#define FAULT_STATE_THREAD_PREEMPTION_THRESHOLD		FAULT_STATE_THREAD_PRIORITY
#define FAULT_STATE_THREAD_NAME						"Fault State Thread"

/**
 * @brief Thread for fault state
 */
TX_THREAD fault_state_thread;

/*
 * function prototypes
 */
void fault_state_thread_entry(ULONG thread_input);

/**
 * @brief 		Initialise fault state thread
 * 
 * @param[in]	stack_pool_ptr 	Pointer to start of application stack area
 * 
 * @return 		See ThreadX return codes
 */
UINT fault_state_thread_init(TX_BYTE_POOL* stack_pool_ptr)
{
	VOID* thread_stack_ptr;

	UINT ret = tx_byte_allocate(stack_pool_ptr, 
								&thread_stack_ptr, 
								FAULT_STATE_THREAD_STACK_SIZE, 
								TX_NO_WAIT);

    if (ret == TX_SUCCESS)
    {
        ret = tx_thread_create(&fault_state_thread, 
								FAULT_STATE_THREAD_NAME, 
								fault_state_thread_entry, 
								0, 
								thread_stack_ptr,
                    			FAULT_STATE_THREAD_STACK_SIZE, 
								FAULT_STATE_THREAD_PRIORITY, 
								FAULT_STATE_THREAD_PREEMPTION_THRESHOLD,
                    			TX_NO_TIME_SLICE, 
								TX_DONT_START);
    }

	return ret;
}

/**
 * @brief 		Fault state thread entry function
 *
 * @param[in]	thread_input	Unused input
 */
void fault_state_thread_entry(ULONG thread_input)
{
	// drive fault output pin low
	HAL_GPIO_WritePin(FAULT_GPIO_Port, FAULT_Pin, GPIO_PIN_RESET);

	// TODO: zero torque request to inverter


	// shut down other threads
	// note: this thread has the highest priority and will not be pre-empted
	sensor_thread_terminate();
	control_thread_terminate();
	can_tx_thread_terminate();
	can_rx_thread_terminate();

	// loop forever, do not leave fault state
	// flash the LED
	while (1)
	{
		HAL_GPIO_TogglePin(RED_LED_GPIO_Port, RED_LED_Pin);
		tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / FAULT_STATE_LED_BLINK_RATE);
	}

}

/**
 * @brief	Enter the fault state
 */
void enter_fault_state()
{
	tx_thread_resume(&fault_state_thread);
	tx_thread_relinquish();
}
