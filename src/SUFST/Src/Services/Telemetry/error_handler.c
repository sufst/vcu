#include <tx_api.h>
#include "error_handler.h"
#include "main.h"
#include <stdatomic.h>
#define ERROR_HANDLER_THREAD_PRIORITY             10
#define ERROR_HANDLER_THREAD_STACK_SIZE           1024
#define ERROR_HANDLER_THREAD_PREEMPTION_THRESHOLD 10                

static void error_handler_thread_entry(ULONG input);

/**
 * @brief 		Creates the error_handler thread
 *
 * @param[in]	stack_pool_ptr	Pointer to start of application stack area
 *
 * @return		See ThreadX return codes
 */
UINT error_handler_init(error_handler_context_t* error_handler_ptr, TX_BYTE_POOL* stack_pool_ptr)
{

    VOID* thread_stack_ptr = NULL;

    UINT tx_status = tx_byte_allocate(stack_pool_ptr,
                                &thread_stack_ptr,
                                ERROR_HANDLER_THREAD_STACK_SIZE,
                                TX_NO_WAIT);

    if (tx_status == TX_SUCCESS)
    {
        tx_status = tx_thread_create(&error_handler_ptr->thread,
                               "Watchdog Thread",
                               error_handler_thread_entry,
                               error_handler_ptr,
                               thread_stack_ptr,
                               ERROR_HANDLER_THREAD_STACK_SIZE,
                               ERROR_HANDLER_THREAD_PRIORITY,
                               ERROR_HANDLER_THREAD_PREEMPTION_THRESHOLD,
                               TX_NO_TIME_SLICE,
                               TX_AUTO_START);
    }

    if(tx_status == TX_SUCCESS)
    {
        tx_status = tx_semaphore_create(&error_handler_ptr->fault_semaphore,
                                "Error Handler Semaphore", 
                                0);
    }

    return tx_status;
}

static void error_handler_thread_entry(ULONG input)
{
    error_handler_context_t* error_handler_ptr = (error_handler_context_t*) input;

    /* Turn on LED by default */
    HAL_GPIO_WritePin(LED_OUT_GPIO_Port,LED_OUT_Pin, GPIO_PIN_SET);
    /* Suspend the thread on semaphore, when fault happen it will give back the semaphore */
    const UINT status = tx_semaphore_get(&error_handler_ptr->fault_semaphore, TX_WAIT_FOREVER);


    while(1)
    {

        switch (error_handler_ptr->error_code)
        {
        case CAN_UNPACK_INIT_ERROR:
            break;

        case CAN_UNPACK_QUEUE_ERROR:
            break;

        case CAN_UNPACK_RTCAN_INIT_ERROR:
            break;
        
        case CAN_UNPACK_STATS_MUTEX_ERROR:
            break;

        case XBEE_COMMS_INIT_ERROR:
            break;

        case XBEE_COMMS_QUEUE_ERROR:
            break;

        default:
            break;
        }
        HAL_GPIO_TogglePin(LED_OUT_GPIO_Port,LED_OUT_Pin);
        // Introduce 500ms delay
        tx_thread_sleep(TX_TIMER_TICKS_PER_SECOND / 2);
    }

}

void critical_error(TX_THREAD* thread_ptr, uint32_t error_code, error_handler_context_t* error_handler)
{

    atomic_store(&error_handler->error_code, error_code);
    tx_semaphore_put(&error_handler->fault_semaphore);

    /* Prioritise error_handler thread and suspend calling thread */
    tx_thread_priority_change(&error_handler->thread, 0, NULL);
    tx_thread_suspend(thread_ptr);
}