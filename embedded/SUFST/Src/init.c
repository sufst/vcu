/***************************************************************************
 * @file   init.c
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-05-20
 * @brief  Initialisation function implementation
 ***************************************************************************/

#include "init.h"

#include "apps.h"
#include "bps.h"

#include "can_rx_thread.h"
#include "can_tx_thread.h"
#include "control_thread.h"
#include "init_thread.h"
#include "pm100.h"
#include "sensor_thread.h"
#include "watchdog_thread.h"

/**
 * @brief       Create and initialise threads
 *
 * @param[in]   stack_pool_ptr  Pointer to start of application stack area
 *
 * @return      See ThreadX return codes
 */
UINT init_threads(TX_BYTE_POOL* stack_pool_ptr)
{
    UINT(*thread_init_funcs[])
    (TX_BYTE_POOL*) = {
        can_rx_thread_create,
        can_tx_thread_create,
        control_thread_create,
        watchdog_thread_create,
        sensor_thread_create,
        init_thread_create,
    };

    const UINT num_threads
        = sizeof(thread_init_funcs) / sizeof(thread_init_funcs[0]);
    UINT ret = TX_SUCCESS;

    for (UINT i = 0; i < num_threads; i++)
    {
        ret = thread_init_funcs[i](stack_pool_ptr);

        if (ret != TX_SUCCESS)
        {
            break;
        }
    }

    return ret;
}

/**
 * @brief       Initialisation pre ready-to-drive
 *
 * @note        Runs both before ready-to-drive and before ThreadX kernel entry
 */
UINT init_pre_rtd(TX_BYTE_POOL* stack_pool_ptr)
{
    bps_init();
    apps_init();

    UINT ret = pm100_init() == PM100_OK ? TX_SUCCESS : TX_START_ERROR;
    return ret;
}

/**
 * @brief Initialisation post ready-to-drive
 */
UINT init_post_rtd()
{
    UINT ret;

#if TRACEX_ENABLE
    ret = trace_init();
    trace_log_event(TRACE_READY_TO_DRIVE_EVENT, 0, 0, 0, 0);
#else
    ret = TX_SUCCESS;
#endif

    return ret;
}