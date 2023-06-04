#include "pm100.h"

/*
 * internal function prototypes
 */
static void pm100_thread_entry(ULONG input);

/**
 * @brief   Initialises the PM100 service
 *
 * @param[in]   pm100_ptr       PM100 context
 * @param[in]   stack_pool_ptr  Memory pool for service thread stack
 * @param[in]   config_ptr      Configuration
 */
status_t pm100_init(pm100_context_t* pm100_ptr,
                    TX_BYTE_POOL* stack_pool_ptr,
                    const config_pm100_t* config_ptr)
{
    pm100_ptr->config_ptr = config_ptr;

    status_t status = STATUS_OK;

    // create service thread
    void* stack_ptr = NULL;
    UINT tx_status = tx_byte_allocate(stack_pool_ptr,
                                      &stack_ptr,
                                      config_ptr->thread.stack_size,
                                      TX_NO_WAIT);

    if (tx_status == TX_SUCCESS)
    {
        tx_status = tx_thread_create(&pm100_ptr->thread,
                                     config_ptr->thread.name,
                                     pm100_thread_entry,
                                     (ULONG) pm100_ptr,
                                     stack_ptr,
                                     config_ptr->thread.stack_size,
                                     config_ptr->thread.priority,
                                     config_ptr->thread.priority,
                                     TX_NO_TIME_SLICE,
                                     TX_AUTO_START);
    }

    if (tx_status != TX_SUCCESS)
    {
        status = STATUS_ERROR;
    }

    // check all ok
    if (status != STATUS_OK)
    {
        tx_thread_terminate(&pm100_ptr->thread);
    }

    return status;
}

/**
 * @brief   PM100 service thread entry function
 */
void pm100_thread_entry(ULONG input)
{
    while (1)
    {
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND);
    }
}

/**
 * @brief       Initiates the precharge sequence
 *
 * @param[in]   pm100_ptr   PM100 context
 */
status_t pm100_start_precharge(pm100_context_t* pm100_ptr)
{
    // TODO: implement precharge activation
    return STATUS_ERROR;
}

/**
 * @brief       Checks if the PM100 is precharged
 *
 * @param[in]   pm100_ptr   PM100 context
 */
bool pm100_is_precharged(pm100_context_t* pm100_ptr)
{
    // TODO: check PM100 states to determine if precharge complete
    return false;
}

/**
 * @brief       Disables the inverter
 *
 * @param[in]   pm100_ptr
 */
status_t pm100_disable(pm100_context_t* pm100_ptr)
{
    // TODO: implement disabling of inverter
    return STATUS_ERROR;
}

/**
 * @brief       Sends a torque request to the PM100
 *
 * @param[in]   pm100_ptr   PM100 context
 * @param[in]   torque      Desired torque
 */
status_t pm100_request_torque(pm100_context_t* pm100_ptr, uint16_t torque)
{
    // TODO: implement torque requests
    return STATUS_ERROR;
}