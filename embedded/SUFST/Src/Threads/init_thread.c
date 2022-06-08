/***************************************************************************
 * @file   init_thread.c
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-06-08
 * @brief  Initialisation thread
 ***************************************************************************/

#include "init_thread.h"
#include "config.h"

#include "ready_to_drive.h"
#include "init.h"

#include "can_tx_thread.h"
#include "control_thread.h"
#include "sensor_thread.h"

#define INIT_THREAD_STACK_SIZE              512
#define INIT_THREAD_PREEMPTION_THRESHOLD    INIT_THREAD_PRIORITY
#define INIT_THREAD_NAME                    "Initialisation Thread"

/**
 * @brief Initialisation thread instance
 */
static TX_THREAD init_thread;

/*
 * function prototypes
 */
static void init_thread_entry(ULONG thread_input);
static void start_threads();

/**
 * @brief 		Creates the initialisation thread
 * 
 * @param[in]	stack_pool_ptr	Pointer to start of application stack area
 * 
 * @return		See ThreadX return codes
 */
UINT init_thread_create(TX_BYTE_POOL* stack_pool_ptr)
{
    VOID* thread_stack_ptr;

    UINT ret = tx_byte_allocate(stack_pool_ptr,
                                &thread_stack_ptr,
                                INIT_THREAD_STACK_SIZE,
                                TX_NO_WAIT);

    if (ret == TX_SUCCESS)
    {
        ret = tx_thread_create(&init_thread,
                               INIT_THREAD_NAME,
                               init_thread_entry,
                               0,
                               thread_stack_ptr,
                               INIT_THREAD_STACK_SIZE,
                               INIT_THREAD_PRIORITY,
                               INIT_THREAD_PREEMPTION_THRESHOLD,
                               TX_NO_TIME_SLICE,
                               TX_AUTO_START);
    }

    return ret;
}

/**
 * @brief       Initialisation thread entry function
 * 
 * @details     This begins running as soon as the RTOS kernel is entered
 *              (immediately following thread creation). The initialisation
 *              thread then has the following responsibilities:
 *              
 *              1. Complete the pre ready-to-drive initialisation.
 *              2. Wait for the ready-to-drive state to be entered.
 *              3. Complete the post ready-to-drive initialisation.
 *              4. Terminate itself and launch all other threads.
 * 
 *              The CAN receive thread should also run at the same time as this
 *              thread
 *              
 *
 * @param[in]	thread_input	Unused input
 */
void init_thread_entry(ULONG thread_input)
{
    init_pre_rtd();
    rtd_wait();
    init_post_rtd();

    start_threads();
}

/**
 * @brief Start all threads which were not auto-started
 */
void start_threads()
{
    UINT (*thread_start_funcs[])() = {
        can_tx_thread_start,
        control_thread_start,
        sensor_thread_start
    };

    const UINT num_to_start = sizeof(thread_start_funcs) / sizeof(thread_start_funcs[0]);

    for (UINT i = 0; i < num_to_start; i++)
    {
        thread_start_funcs[i]();
    }
}