/***************************************************************************
 * @file   fault.c
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-05-21
 * @brief  Fault handling implementation
 ***************************************************************************/

#include "fault.h"

#include "config.h"
#include "tx_api.h"

#include "trace.h"

/**
 * @brief Minor fault queue
 */
extern TX_QUEUE minor_fault_queue;

/**
 * @brief Critical fault queue
 */
extern TX_QUEUE critical_fault_queue;

/**
 * @brief Fault watchdog thread
 */
extern TX_THREAD watchdog_thread;

/**
 * @brief Fault semaphore
 */
extern TX_SEMAPHORE fault_semaphore;

/**
 * @brief       Register minor fault
 *
 * @param[in]   fault   Type of fault
 */
void minor_fault(minor_fault_t fault)
{
    // log fault occurrence
    trace_log_event(TRACE_MINOR_FAULT, (ULONG) fault, 0, 0, 0);

    // send message to watchdog thread
    ULONG queue_message = (ULONG) fault;
    tx_queue_send(&minor_fault_queue, (VOID*) &queue_message, TX_WAIT_FOREVER);
    tx_semaphore_put(&fault_semaphore);
}

/**
 * @brief       Register critical fault
 *
 * @param[in]   fault   Type of fault
 */
void critical_fault(critical_fault_t fault)
{
    // log fault occurrence
    trace_log_event(TRACE_CRITICAL_FAULT, (ULONG) fault, 0, 0, 0);

    // send message to watchdog thread
    ULONG queue_message = (ULONG) fault;
    tx_queue_send(&critical_fault_queue,
                  (VOID*) &queue_message,
                  TX_WAIT_FOREVER);
    tx_semaphore_put(&fault_semaphore);

    // ensure watchdog thread runs immediately
    tx_thread_priority_change(&watchdog_thread,
                              WATCHDOG_THREAD_PRIORITY_ELEVATED,
                              NULL);
    tx_thread_relinquish();
}