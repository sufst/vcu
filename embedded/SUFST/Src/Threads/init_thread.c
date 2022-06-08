/***************************************************************************
 * @file   init_thread.c
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-06-08
 * @brief  Initialisation thread
 ***************************************************************************/

#include "init_thread.h"
#include "config.h"

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
 * @param[in]	thread_input	Unused input
 */
void init_thread_entry(ULONG thread_input)
{
    // ...
}