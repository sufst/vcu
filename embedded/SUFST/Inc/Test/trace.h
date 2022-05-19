/***************************************************************************
 * @file   trace.h
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-05-14
 * @brief  TraceX profiling interface
 * 
 * @note   Dump the trace buffer (see: trace.c) to a .trx file using the 
 *         debugger and open it in the TraceX app.
***************************************************************************/

#include "config.h"
#include "tx_api.h"

#if TRACEX_ENABLE

// trace configuration
#define TRACEX_BUFFER_SIZE      10240UL
#define TRACEX_REGISTRY_ENTRIES 32
#define TRACEX_ENABLED_EVENTS   (  TX_TRACE_BYTE_POOL_EVENTS \
                                 | TX_TRACE_MUTEX_EVENTS \
                                 | TX_TRACE_QUEUE_EVENTS \
                                 | TX_TRACE_SEMAPHORE_EVENTS \
                                 | TX_TRACE_THREAD_EVENTS \
                                 | TX_TRACE_TIME_EVENTS \
                                 | TX_TRACE_TIMER_EVENTS \
                                 | TX_TRACE_USER_EVENTS )

// custom trace events (0x1000 to 0xFFFF)
#define TRACE_DUMMY_EVENT               0x0000

#define TRACE_READY_TO_DRIVE_EVENT      0x1000

#define TRACE_THROTTLE_INPUT_EVENT      0x2002
#define TRACE_TORQUE_REQUEST_EVENT      0x2003

#define TRACE_INVERTER_ERROR            0xE001

// #define TRACE_GENERIC_FAULT_EVENT       0xF001
// #define TRACE_SIGNAL_FAULT_EVENT        0xF002
// #define TRACE_CAN_FAULT_EVENT           0xF003

// function prototypes
UINT trace_init();
VOID trace_log_event(ULONG event_id, ULONG data_1, ULONG data_2, ULONG data_3, ULONG data_4);

#else 
#define trace_log_event(i, d1, d2, d3, d4) // empty macro deletes function calls
#endif