/***************************************************************************
 * @file   control_thread.c
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2021-11-30
 * @brief  Control thread implementation
 ***************************************************************************/

#include "control_thread.h"

#include "config.h"
#include "tx_api.h"

#include "fault.h"

#include "can_tx_thread.h"
#include "driver_profiles.h"

#define CONTROL_THREAD_STACK_SIZE           1024
#define CONTROL_THREAD_PREEMPTION_THRESHOLD CONTROL_THREAD_PRIORITY
#define CONTROL_THREAD_NAME                 "Control Thread"

#define THROTTLE_INPUT_QUEUE_ITEM_SIZE      TX_1_ULONG
#define THROTTLE_INPUT_QUEUE_SIZE           2
#define THROTTLE_INPUT_QUEUE_NAME           "Throttle Input Queue"

/**
 * @brief Thread for control task
 */
TX_THREAD control_thread;

/**
 * @brief Throttle input queue
 */
TX_QUEUE throttle_input_queue;

/**
 * @brief Throttle input queue memory area
 */
static ULONG throttle_input_queue_mem[THROTTLE_INPUT_QUEUE_SIZE
                                      * THROTTLE_INPUT_QUEUE_ITEM_SIZE];

/*
 * function prototypes
 */
void control_thread_entry(ULONG thread_input);

/**
 * @brief 		Create control thread
 *
 * @param[in]	stack_pool_ptr 	Pointer to start of application stack area
 *
 * @return 		See ThreadX return codes
 */
UINT control_thread_create(TX_BYTE_POOL* stack_pool_ptr)
{
    // create thread
    VOID* thread_stack_ptr;

    UINT ret = tx_byte_allocate(stack_pool_ptr,
                                &thread_stack_ptr,
                                CONTROL_THREAD_STACK_SIZE,
                                TX_NO_WAIT);

    if (ret == TX_SUCCESS)
    {
        ret = tx_thread_create(&control_thread,
                               CONTROL_THREAD_NAME,
                               control_thread_entry,
                               0,
                               thread_stack_ptr,
                               CONTROL_THREAD_STACK_SIZE,
                               CONTROL_THREAD_PRIORITY,
                               CONTROL_THREAD_PREEMPTION_THRESHOLD,
                               TX_NO_TIME_SLICE,
                               TX_DONT_START);
    }

    // create throttle input queue
    if (ret == TX_SUCCESS)
    {
        ret = tx_queue_create(&throttle_input_queue,
                              THROTTLE_INPUT_QUEUE_NAME,
                              THROTTLE_INPUT_QUEUE_ITEM_SIZE,
                              throttle_input_queue_mem,
                              sizeof(throttle_input_queue_mem));
    }

    return ret;
}

/**
 * @brief	Starts the control thread
 *
 * @return	See ThreadX return codes
 */
UINT control_thread_start()
{
    return tx_thread_resume(&control_thread);
}

/**
 * @brief 	Terminate control thread
 *
 * @return 	See ThreadX return codes
 */
UINT control_thread_terminate()
{
    return tx_thread_terminate(&control_thread);
}

/**
 * @brief Thread entry function for control_thread
 *
 * @param[in]	thread_input	(Unused) thread input
 */
void control_thread_entry(ULONG thread_input)
{
    (void) thread_input;

    // look-up the driver profile
    // (note this is fixed at compile time at the moment)
    const driver_profile_t* driver_profile_ptr;

    if (driver_profile_lookup(&driver_profile_ptr, SELECTED_DRIVER_PROFILE)
        != DRIVER_PROFILE_FOUND)
    {
        critical_fault(CRITICAL_FAULT_DRIVER_PROFILE_NOT_FOUND);
    }

    // loop forever
    while (1)
    {
        // wait for a message to enter the control input queue
        // -> thread suspended until message received
        ULONG throttle_input = 0;

        if (tx_queue_receive(&throttle_input_queue,
                             &throttle_input,
                             TX_WAIT_FOREVER)
            == TX_SUCCESS)
        {
            // apply the thottle curve
            UINT torque_request
                = apply_torque_map(driver_profile_ptr, throttle_input);

            // create and send the torque request to the CAN thread
            UINT ret = tx_queue_send(&torque_request_queue,
                                     &torque_request,
                                     TX_NO_WAIT);

            if (ret == TX_QUEUE_FULL)
            {
                critical_fault(CRITICAL_FAULT_QUEUE_FULL);
            }
        }
    }
}