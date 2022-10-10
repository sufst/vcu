/***************************************************************************
 * @file   sensor_thread.c
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2021-11-30
 * @brief  Sensor thread
 ***************************************************************************/

#include "sensor_thread.h"

#include <stdbool.h>
#include <stdint.h>

#include "fault.h"
#include "testbench.h"
#include "trace.h"

#include "adc.h"
#include "apps.h"
#include "gpio.h"

#include "control_thread.h"
#include "shutdown.h"

#define SENSOR_THREAD_STACK_SIZE           1024
#define SENSOR_THREAD_PREEMPTION_THRESHOLD SENSOR_THREAD_PRIORITY
#define SENSOR_THREAD_NAME                 "Sensor Thread"
#define SENSOR_THREAD_TICK_RATE            100 // times per second

/**
 * @brief Thread managing sensor measurements and inputs
 */
TX_THREAD sensor_thread;

/**
 * @brief Timer handling thread wakeup tick
 */
TX_TIMER sensor_thread_tick_timer;

/*
 * function prototypes
 */
void sensor_thread_entry(ULONG thread_input);

/**
 * @brief 		Initialise sensor thread
 *
 * @param[in]	stack_pool_ptr	Pointer to start of application stack area
 *
 * @return		See ThreadX return codes
 */
UINT sensor_thread_create(TX_BYTE_POOL* stack_pool_ptr)
{
    VOID* thread_stack_ptr;

    UINT ret = tx_byte_allocate(stack_pool_ptr,
                                &thread_stack_ptr,
                                SENSOR_THREAD_STACK_SIZE,
                                TX_NO_WAIT);

    if (ret == TX_SUCCESS)
    {
        ret = tx_thread_create(&sensor_thread,
                               SENSOR_THREAD_NAME,
                               sensor_thread_entry,
                               0,
                               thread_stack_ptr,
                               SENSOR_THREAD_STACK_SIZE,
                               SENSOR_THREAD_PRIORITY,
                               SENSOR_THREAD_PREEMPTION_THRESHOLD,
                               TX_NO_TIME_SLICE,
                               TX_DONT_START);
    }

    return ret;
}

/**
 * @brief	Starts the sensor thread
 *
 * @return 	See ThreadX return codes
 */
UINT sensor_thread_start()
{
    return tx_thread_resume(&sensor_thread);
}

/**
 * @brief 	Terminate sensor thread
 *
 * @return 	See ThreadX return codes
 */
UINT sensor_thread_terminate()
{
    return tx_thread_terminate(&sensor_thread);
}

/**
 * @brief Sensor thread tick function
 */
static void sensor_thread_tick(ULONG input)
{
    tx_thread_resume(&sensor_thread);
}

/**
 * @brief Sensor thread entry function
 *
 * @param[in]	thread_input	Unused input
 */
void sensor_thread_entry(ULONG thread_input)
{
    // prevent compiler warnings as input is not used for the moment
    (VOID) thread_input;

    // create timer
    UINT ret
        = tx_timer_create(&sensor_thread_tick_timer,
                          "Sensor Thread Tick Timer",
                          sensor_thread_tick,
                          0,
                          TX_TIMER_TICKS_PER_SECOND / SENSOR_THREAD_TICK_RATE,
                          TX_TIMER_TICKS_PER_SECOND / SENSOR_THREAD_TICK_RATE,
                          TX_NO_ACTIVATE);

    if (ret != TX_SUCCESS)
    {
        return;
    }

    // loop forever
    while (1)
    {
        // check_shutdown();

        // check for fault state
#if RUN_FAULT_STATE_TESTBENCH
        testbench_fault_state();
#endif

        // read APPS
#if RUN_APPS_TESTBENCH
        uint32_t apps_input = testbench_apps_input();
#else
        ULONG apps_input = (ULONG) apps_read();
#endif

        // transmit APPS input to control thread
        ret = tx_queue_send(&apps_input_queue,
                            (ULONG*) &apps_input,
                            TX_NO_WAIT);

        if (ret == TX_QUEUE_FULL)
        {
            critical_fault(CRITICAL_FAULT_QUEUE_FULL);
        }

        // sleep thread to allow other threads to run
        tx_timer_activate(&sensor_thread_tick_timer);
        tx_thread_suspend(&sensor_thread);
    }
}