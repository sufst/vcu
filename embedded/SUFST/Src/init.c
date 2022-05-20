/***************************************************************************
 * @file   init.c
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-05-20
 * @brief  Initialisation function implementation
 ***************************************************************************/

#include "init.h"

#include "can_rx_thread.h"
#include "can_tx_thread.h"
#include "control_thread.h"
#include "fault_state_thread.h"
#include "sensor_thread.h"

UINT init_threads(TX_BYTE_POOL* stack_pool_ptr);

/**
 * @brief       Initialisation pre ready-to-drive
 * 
 * @param[in]   stack_pool_ptr  Pointer to start of application stack area
 */
void init_pre_rtd(TX_BYTE_POOL* stack_pool_ptr)
{
    init_threads(stack_pool_ptr);
}

/**
 * @brief Initialisation post ready-to-drive
 */
void init_post_rtd()
{

}

/**
 * @brief       Create and initialise threads
 * 
 * @param[in]   stack_pool_ptr  Pointer to start of application stack area
 * 
 * @return      UINT: See ThreadX return codes
 */
UINT init_threads(TX_BYTE_POOL* stack_pool_ptr)
{
    UINT (*thread_init_funcs[])(TX_BYTE_POOL*) = {
        can_rx_thread_init,
        can_tx_thread_init,
        control_thread_init,
        fault_state_thread_init,
        sensor_thread_init,
    };

    const UINT num_threads = sizeof(thread_init_funcs) / sizeof(thread_init_funcs[0]);
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