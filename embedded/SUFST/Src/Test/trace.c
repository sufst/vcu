/***************************************************************************
 * @file   trace.c
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-05-14
 * @brief  TraceX profiling interface
 ***************************************************************************/

#include "trace.h"

#include "config.h"

#if TRACEX_ENABLE

// trace memory
static UCHAR trace_buffer[TRACEX_BUFFER_SIZE];

// trace buffer full callback
static VOID trace_buffer_full_callback(VOID* trace_buffer_start);

/**
 * @brief Initialise trace
 */
UINT trace_init()
{
    UINT status = tx_trace_enable(trace_buffer,
                                  TRACEX_BUFFER_SIZE,
                                  TRACEX_REGISTRY_ENTRIES);

    // remove all events from trace
    if (status == TX_SUCCESS)
    {
        status = tx_trace_event_filter(TX_TRACE_ALL_EVENTS);
    }

    // add events which we actually want
    if (status == TX_SUCCESS)
    {
        status = tx_trace_event_unfilter((ULONG) TRACEX_ENABLED_EVENTS);
    }

    // callback
    if (status == TX_SUCCESS)
    {
        status = tx_trace_buffer_full_notify(trace_buffer_full_callback);
    }

    return status;
}

/**
 * @brief   Log an event to the trace
 *
 * @return
 */
VOID trace_log_event(ULONG event_id,
                     ULONG data_1,
                     ULONG data_2,
                     ULONG data_3,
                     ULONG data_4)
{
    tx_trace_user_event_insert(TX_TRACE_USER_EVENT_START + event_id,
                               data_1,
                               data_2,
                               data_3,
                               data_4);
}

/**
 * @brief       Callback when trace buffer is full
 *
 * @param[in]   trace_buffer_start  Start of trace buffer
 */
VOID trace_buffer_full_callback(VOID* trace_buffer_start)
{
    // breakpoint here!
    // to dump trace with gdb (with buffer size of <size>):
    // dump binary memory ~/Desktop/file.trx trace_buffer (trace_buffer +
    // <size>)
    tx_trace_disable();
}

#endif