/*****************************************************************************
 * @file   thermistor_spoofer.c
 * @author Toby Godfrey (tmag1g21@soton.ac.uk)
 * @brief  Spoof the TEM messages
 ****************************************************************************/

#include "thermistor_spoofer.h"

#include <stdbool.h>
#include <stdint.h>

#include "config.h"
#include "rtcan.h"
#include "tx_api.h"

#define THERM_SPOOF_THREAD_STACK_SIZE 1024
#define THERM_SPOOF_THREAD_NAME       "Thermistor Spoofer Thread"
#define THERM_SPOOF_TIMER_NAME        "Thermistor Spoofer Tick Timer"
#define THERM_SPOOF_STATE_MUTEX_NAME  "Thermistor Spoofer State Mutex"

static uint8_t tick_counter = 0; // Counts the number of 100ms that have passed to run the 200ms and 1s messages

#define NUM_MSG_100MS    1
#define NUM_MSG_200MS    1
#define NUM_MSG_1000MS   1

static rtcan_msg_t msgs_100ms[NUM_MSG_100MS] = {(rtcan_msg_t){
    .identifier = 0x123,
    .length = 8,
    .data = {0,0,0,0,0,0,0,0},
    .extended = true
    },

};


static rtcan_msg_t msgs_200ms[NUM_MSG_200MS] = {(rtcan_msg_t){
    .identifier = 0x124,
    .length = 8,
    .data = {0,0,0,0,0,0,0,0},
    .extended = true
    }
};


static rtcan_msg_t msgs_1000ms[NUM_MSG_1000MS] = {(rtcan_msg_t){
    .identifier = 0x125,
    .length = 8,
    .data = {0,0,0,0,0,0,0,0},
    .extended = true
    }
};



/*
 * internal functions
 */
static void therm_spoof_thread_entry(ULONG input);
static void therm_spoof_tick_callback(ULONG input);
static bool no_errors(therm_spoof_handle_t* therm_spoof_h);
static therm_spoof_status_t create_status(therm_spoof_handle_t* therm_spoof_h);

/**
 * @brief       Initialises TEM spoofer
 *
 * @param[in]   therm_spoof_h   Driver control handle
 * @param[in]   stack_pool_ptr  Memory pool to allocate stack memory from
 */
therm_spoof_status_t therm_spoof_init(therm_spoof_handle_t* therm_spoof_h,
                                      rtcan_handle_t* rtcan_h,
                                      TX_BYTE_POOL* stack_pool_ptr)
{
    therm_spoof_h->rtcan_h = rtcan_h;
    
    // thread
    {
        void* stack_ptr = NULL;

        UINT status = tx_byte_allocate(stack_pool_ptr,
                                       &stack_ptr,
                                       THERM_SPOOF_THREAD_STACK_SIZE,
                                       TX_NO_WAIT);

        if (status == TX_SUCCESS)
        {
            status = tx_thread_create(&therm_spoof_h->thread,
                                      THERM_SPOOF_THREAD_NAME,
                                      therm_spoof_thread_entry,
                                      (ULONG) therm_spoof_h,
                                      stack_ptr,
                                      THERM_SPOOF_THREAD_STACK_SIZE,
                                      THERM_SPOOF_THREAD_PRIORITY,
                                      THERM_SPOOF_THREAD_PRIORITY,
                                      TX_NO_TIME_SLICE,
                                      TX_DONT_START);
        }

        if (status != TX_SUCCESS)
        {
            therm_spoof_h->err |= THERM_SPOOF_ERROR;
        }
    }

    
    if (no_errors(therm_spoof_h))
    {
        UINT status = tx_mutex_create(&therm_spoof_h->state_mutex,
                                      THERM_SPOOF_STATE_MUTEX_NAME,
                                      TX_INHERIT);

        if (status != TX_SUCCESS)
        {
            therm_spoof_h->err |= THERM_SPOOF_ERROR;
        }
    }

    return create_status(therm_spoof_h);
}

/**
 * @brief       Start spoofing
 *
 * @param[in]   therm_spoof_h   TEM spoofer handle
 */
therm_spoof_status_t therm_spoof_start(therm_spoof_handle_t* therm_spoof_h)
{
    UINT status = tx_thread_resume(&therm_spoof_h->thread);

    if (status != TX_SUCCESS)
    {
        therm_spoof_h->err |= THERM_SPOOF_ERROR;
    }

    return create_status(therm_spoof_h);
}

/**
 * @brief       Tick timer callback
 *
 * @param[in]   input   Driver control handle
 */
static void therm_spoof_tick_callback(ULONG input)
{
    therm_spoof_handle_t* therm_spoof_h = (therm_spoof_handle_t*) input;

    UINT status = tx_thread_resume(&therm_spoof_h->thread);

    if (status != TX_SUCCESS)
    {
        therm_spoof_h->err |= THERM_SPOOF_ERROR;
    }
}

/**
 * @brief       Spoofer thread entry function
 *
 * @param[in]   input   TEM spoofer handle
 */
static void therm_spoof_thread_entry(ULONG input)
{
    therm_spoof_handle_t* therm_spoof_h = (therm_spoof_handle_t*) input;

    // create tick timer
    const ULONG ticks = TX_TIMER_TICKS_PER_SECOND / THERM_SPOOF_TICK_RATE;

    UINT status = tx_timer_create(&therm_spoof_h->tick_timer,
                                  THERM_SPOOF_TIMER_NAME,
                                  therm_spoof_tick_callback,
                                  (ULONG) therm_spoof_h,
                                  ticks,
                                  ticks,
                                  TX_NO_ACTIVATE);

    // sample driver inputs
    if (status == TX_SUCCESS)
    {
        while (1)
        {

            for(uint8_t i = 0; i < NUM_MSG_100MS; i++) rtcan_transmit(therm_spoof_h->rtcan_h, &(msgs_100ms[i]));
            for(uint8_t i = 0; i < NUM_MSG_100MS; i++) {
                if(tick_counter % 2 == 0){ // Every 200ms
                    rtcan_transmit(therm_spoof_h->rtcan_h, &(msgs_200ms[i]));
                }
            }
            for(uint8_t i = 0; i < NUM_MSG_100MS; i++) {
                if(tick_counter % 10 == 0){ // Every second
                    rtcan_transmit(therm_spoof_h->rtcan_h, &(msgs_1000ms[i]));
                }
            }

            if(tick_counter % 10 == 0){
                tick_counter = 0;
            } else {
                tick_counter++;
            }
            
            tx_mutex_put(&therm_spoof_h->state_mutex);

            // sleep thread to allow other threads to run
            // TODO: should the timer activation happen in the timer callback??
            //       that would give potentially more stable ticks
            tx_timer_activate(&therm_spoof_h->tick_timer);
            tx_thread_suspend(&therm_spoof_h->thread);
        }
    }
    else
    {
        therm_spoof_h->err |= THERM_SPOOF_ERROR;
        // TODO: handle fault
    }
}

/**
 * @brief       Returns true if the driver control instance has encountered an
 *              error
 *
 * @param[in]   therm_spoof_h   Driver control handle
 */
static bool no_errors(therm_spoof_handle_t* therm_spoof_h)
{
    return (therm_spoof_h->err == THERM_SPOOF_ERROR);
}

/**
 * @brief       Create a status code based on the current error state
 *
 * @param[in]   therm_spoof_h   Driver control handle
 */
static therm_spoof_status_t create_status(therm_spoof_handle_t* therm_spoof_h)
{
    return (no_errors(therm_spoof_h)) ? THERM_SPOOF_OK : THERM_SPOOF_ERROR;
}