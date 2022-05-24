/***************************************************************************
 * @file   	watchdog_thread.c
 * @author 	Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   	2022-05-21
 * @brief  	Watchdog thread implementation
 * 
 * @details	This thread will wake up:
 * 			- Immediately when an item enters the critical fault queue
 *          - When next scheduled when an item enters the minor fault queue
 *          - Every WATCHDOG_THREAD_WAKE_TIMEOUT seconds regardless of fault
 ***************************************************************************/

#include "watchdog_thread.h"
#include "config.h"

#include "can_tx_thread.h"
#include "can_rx_thread.h"
#include "control_thread.h"
#include "sensor_thread.h"

#include "fault.h"
#include "gpio.h"
#include "pm100.h"

#define WATCHDOG_THREAD_STACK_SIZE					512
#define WATCHDOG_THREAD_PREEMPTION_THRESHOLD		WATCHDOG_THREAD_PRIORITY
#define WATCHDOG_THREAD_NAME						"Watchdog Thread"
#define WATCHDOG_THREAD_WAKE_TIMEOUT				TX_TIMER_TICKS_PER_SECOND / 2

#define CRITICAL_FAULT_QUEUE_ITEM_SIZE				TX_1_ULONG
#define CRITICAL_FAULT_QUEUE_SIZE					5
#define CRITICAL_FAULT_QUEUE_NAME					"Critical Fault Queue"

#define MINOR_FAULT_QUEUE_ITEM_SIZE					TX_1_ULONG
#define MINOR_FAULT_QUEUE_SIZE						10
#define MINOR_FAULT_QUEUE_NAME						"Minor Fault Queue"

#define FAULT_SEMAPHORE_NAME						"Fault Semaphore"

/**
 * @brief Watchdog thread
 */
TX_THREAD watchdog_thread;

/**
 * @brief Minor fault queue
 */
TX_QUEUE minor_fault_queue;

/**
 * @brief Minor fault queue memory area
 */
static ULONG minor_fault_queue_mem[MINOR_FAULT_QUEUE_SIZE];

/**
 * @brief Critical fault queue
 */
TX_QUEUE critical_fault_queue;

/**
 * @brief Critical fault queue memory area
 */
static ULONG critical_fault_queue_mem[CRITICAL_FAULT_QUEUE_SIZE];

/**
 * @brief Fault semaphore
 */
TX_SEMAPHORE fault_semaphore;

/*
 * function prototypes
 */
void watchdog_thread_entry(ULONG thread_input);
void critical_fault_handler(critical_fault_t fault);
void minor_fault_handler(minor_fault_t fault);
void check_system_state();
void check_pm100_state();

/**
 * @brief 		Initialise fault state thread
 * 
 * @param[in]	stack_pool_ptr 	Pointer to start of application stack area
 * 
 * @return 		See ThreadX return codes
 */
UINT watchdog_thread_init(TX_BYTE_POOL* stack_pool_ptr)
{
	// create the thread
	VOID* thread_stack_ptr;

	UINT ret = tx_byte_allocate(stack_pool_ptr, 
								&thread_stack_ptr, 
								WATCHDOG_THREAD_STACK_SIZE, 
								TX_NO_WAIT);

    if (ret == TX_SUCCESS)
    {
        ret = tx_thread_create(&watchdog_thread, 
								WATCHDOG_THREAD_NAME, 
								watchdog_thread_entry, 
								0, 
								thread_stack_ptr,
                    			WATCHDOG_THREAD_STACK_SIZE, 
								WATCHDOG_THREAD_PRIORITY, 
								WATCHDOG_THREAD_PREEMPTION_THRESHOLD,
                    			TX_NO_TIME_SLICE, 
								TX_AUTO_START);
    }

	// create queues
	if (ret == TX_SUCCESS)
	{
		ret = tx_queue_create(&minor_fault_queue,
							  MINOR_FAULT_QUEUE_NAME,
							  MINOR_FAULT_QUEUE_ITEM_SIZE,
							  minor_fault_queue_mem,
							  sizeof(minor_fault_queue_mem) / sizeof(minor_fault_queue_mem[0]));
	}

	if (ret == TX_SUCCESS)
	{
		ret = tx_queue_create(&critical_fault_queue,
							  CRITICAL_FAULT_QUEUE_NAME,
							  CRITICAL_FAULT_QUEUE_ITEM_SIZE,
							  critical_fault_queue_mem,
							  sizeof(critical_fault_queue_mem) / sizeof(critical_fault_queue_mem[0]));
	}

	// create semaphore
	if (ret == TX_SUCCESS)
	{
		ret = tx_semaphore_create(&fault_semaphore, FAULT_SEMAPHORE_NAME, 0);
	}

	return ret;
}

/**
 * @brief 		Watchdog thread entry function
 *
 * @param[in]	thread_input	Unused input
 */
void watchdog_thread_entry(ULONG thread_input)
{
	(void) thread_input;

	while (1)
	{
		const UINT status = tx_semaphore_get(&fault_semaphore, WATCHDOG_THREAD_WAKE_TIMEOUT);

		switch (status)
		{
			// fault has occurred
			case TX_SUCCESS:
			{
				// check for critical faults first
				critical_fault_t critical_fault;

				if (tx_queue_receive(&critical_fault_queue, (VOID*) &critical_fault, TX_NO_WAIT) == TX_SUCCESS)
				{
					critical_fault_handler(critical_fault);
					break;
				}

				// then check for minor faults
				minor_fault_t minor_fault;

				if (tx_queue_receive(&minor_fault_queue, (VOID*) &minor_fault, TX_NO_WAIT) == TX_SUCCESS)
				{
					minor_fault_handler(minor_fault);
					break;
				}

				break;
			}

			// timed out waiting for fault semaphore, run generic checks
			case TX_NO_INSTANCE:
			{
				check_system_state();
				break;
			}

			default:
				break;
		}

	}

}

/**
 * @brief 		Critical fault handler
 * 
 * @param[in]	fault	Fault type
 */
void critical_fault_handler(critical_fault_t fault)
{
	// immediately drive fault pin low and disable inverter
	HAL_GPIO_WritePin(FAULT_GPIO_Port, FAULT_Pin, GPIO_PIN_RESET);
	pm100_disable();

	// shut down driver input and control
	sensor_thread_terminate();
	control_thread_terminate();

	// the end
	while (1)
	{
		HAL_GPIO_TogglePin(RED_LED_GPIO_Port, RED_LED_Pin);
		tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / FAULT_STATE_LED_BLINK_RATE);
	}

}

/**
 * @brief 		Minor fault handler
 * 
 * @param[in]	fault	Fault type
 */
void minor_fault_handler(minor_fault_t fault)
{

}

/**
 * @brief Check system state for potential faults
 */
void check_system_state()
{
	check_pm100_state();
}

/**
 * @brief 	Check inverter state for faults
 * 
 * @details	Checks for presence of "run faults" (see PM100 datasheet), but not which 
 * 			specific fault occurred (this can be determined from the PM100 state with
 * 			the debugger).
 */
void check_pm100_state()
{
	const pm100_state_index_t fault_indexes[] = {
		PM100_RUN_FAULT_LO,
		PM100_RUN_FAULT_HI
	};

	uint32_t state_value;

	for (uint32_t i = 0; i < (sizeof(fault_indexes) / sizeof(fault_indexes[0])); i++)
	{
		pm100_read_state(fault_indexes[i], &state_value);
		
		if (state_value != 0)
		{
			critical_fault(CRITICAL_FAULT_INVERTER_INTERNAL);
		}
	}

}