/***************************************************************************
 * @file   init.c
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-05-20
 * @brief  Initialisation function implementation
 ***************************************************************************/

#include "init.h"

#include "config.h"

#include "trace.h"

#include "apps.h"
#include "bps.h"

/**
 * @brief       Initialisation pre ready-to-drive
 *
 * @note        Runs both before ready-to-drive and before ThreadX kernel entry
 */
UINT init_pre_rtd(TX_BYTE_POOL* stack_pool_ptr)
{
    bps_init();
    apps_init();

    return TX_SUCCESS;
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