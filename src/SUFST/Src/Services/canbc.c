/***************************************************************************
 * @file    canbc.h
 * @author  Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief   CAN broadcast implementation
 ***************************************************************************/

#include "canbc.h"

#include "can_database.h"

#define CANBC_THREAD_STACK_SIZE  1024
#define CANBC_THREAD_NAME        "CANBC Thread"
#define CANBC_STATE_MUTEX_NAME   "CANBC State Mutex"
#define CANBC_DEFAULT_SLEEP_TIME (TX_TIMER_TICKS_PER_SECOND / 2)

/*
 * function prototypes
 */

static void canbc_thread_entry(ULONG input);
static void sleep_till_next_bc(canbc_handle_t* canbc_h, uint32_t start_time);
static void send_bc_messages(canbc_handle_t* canbc_h);

/**
 * @brief       Initialise CANBC service
 *
 * @param[in]   canbc_h         CANBC handle
 * @param[in]   rtcan_h         RTCAN handle
 * @param[in]   period          Broadcast period in ticks
 * @param[in]   priority        CANBC service thread priority
 * @param[in]   stack_pool_ptr  Application memory pool
 */
void canbc_init(canbc_handle_t* canbc_h,
                rtcan_handle_t* rtcan_h,
                uint32_t period,
                uint32_t priority,
                TX_BYTE_POOL* stack_pool_ptr)
{
    canbc_h->rtcan_h = rtcan_h;
    canbc_h->bc_period = period;
    canbc_h->rolling_counter = 0;

    // create service thread
    void* stack_ptr = NULL;
    UINT tx_status = tx_byte_allocate(stack_pool_ptr,
                                      &stack_ptr,
                                      CANBC_THREAD_STACK_SIZE,
                                      TX_NO_WAIT);

    if (tx_status == TX_SUCCESS)
    {
        tx_status = tx_thread_create(&canbc_h->thread,
                                     CANBC_THREAD_NAME,
                                     canbc_thread_entry,
                                     (ULONG) canbc_h,
                                     stack_ptr,
                                     CANBC_THREAD_STACK_SIZE,
                                     priority,
                                     priority,
                                     TX_NO_TIME_SLICE,
                                     TX_DONT_START);
    }

    // create state mutex
    if (tx_status == TX_SUCCESS)
    {
        tx_status
            = tx_mutex_create(&canbc_h->state_mutex, CANBC_STATE_MUTEX_NAME, 0);
    }

    // start service
    if (tx_status == TX_SUCCESS)
    {
        tx_thread_resume(&canbc_h->thread);
    }
}

/**
 * @brief       CANBC service thread
 *
 * @details     Wakes periodically to broadcast to the CAN bus
 *
 * @param[in]   input   CANBC handle
 */
static void canbc_thread_entry(ULONG input)
{
    canbc_handle_t* canbc_h = (canbc_handle_t*) input;

    while (1)
    {
        uint32_t start_time = tx_time_get();
        send_bc_messages(canbc_h);
        sleep_till_next_bc(canbc_h, start_time);
    }
}

/**
 * @brief       Sends broadcast messages via RTCAN
 *
 * @note        This approach doesn't scale particularly well, but at the
 *              moment there are a limited number of broadcast states so this
 *              implementation is the simplest.
 *
 * @param[in]   canbc_h     CANBC handle
 */
static void send_bc_messages(canbc_handle_t* canbc_h)
{
    UINT tx_status = tx_mutex_get(&canbc_h->state_mutex, TX_WAIT_FOREVER);

    if (tx_status == TX_SUCCESS)
    {
        // driver inputs, units are x10
        {
            rtcan_msg_t message
                = {.identifier = CAN_DATABASE_VCU_DRIVER_INPUTS_FRAME_ID,
                   .length = CAN_DATABASE_VCU_DRIVER_INPUTS_LENGTH};

            struct can_database_vcu_driver_inputs_t driver_inputs = {
                .vcu_apps = 10 * canbc_h->states.apps_reading,
                .vcu_bps = 10 * canbc_h->states.bps_reading,
            };

            can_database_vcu_driver_inputs_pack(message.data,
                                                &driver_inputs,
                                                message.length);

            rtcan_transmit(canbc_h->rtcan_h, &message);
        }

        // VCU internal states and rolling counter
        {
            rtcan_msg_t message = {
                .identifier = CAN_DATABASE_VCU_INTERNAL_STATES_FRAME_ID,
                .length = CAN_DATABASE_VCU_INTERNAL_STATES_LENGTH,
            };

            struct can_database_vcu_internal_states_t internal_states
                = {.vcu_ready_to_drive = canbc_h->states.vcu_state.r2d,
                   .vcu_shutdown = canbc_h->states.vcu_state.shutdown,
                   .vcu_rolling_counter = canbc_h->rolling_counter};

            can_database_vcu_internal_states_pack(message.data,
                                                  &internal_states,
                                                  message.length);

            rtcan_transmit(canbc_h->rtcan_h, &message);
        }

        canbc_h->rolling_counter++;
        tx_mutex_put(&canbc_h->state_mutex);
    }
}

/**
 * @brief       Suspends CANBC thread until the next broadcast is due
 *
 * @param[in]   canbc_h     CANBC handle
 * @param[in]   start_time  Timestamp at which last broadcast event started
 */
static void sleep_till_next_bc(canbc_handle_t* canbc_h, uint32_t start_time)
{
    uint32_t run_time = tx_time_get() - start_time;
    uint32_t sleep_time = canbc_h->bc_period - run_time;

    // check for overflow or zero, just in case
    if (run_time >= canbc_h->bc_period)
    {
        sleep_time = CANBC_DEFAULT_SLEEP_TIME;
    }

    tx_thread_sleep(sleep_time);
}

/**
 * @brief       Locks the broadcast states for editing
 *
 * @details     If the CANBC thread or another system service has the lock and
 *              the timeout is reached, NULL is returned.
 *
 *              `canbc_unlock_state()` must be called as soon as possible after
 *              editing states.
 *
 *
 * @param[in]   canbc_h     CANBC handle
 * @param[in]   timeout     Timeout in ticks to acquire lock
 */
canbc_states_t* canbc_lock_state(canbc_handle_t* canbc_h, uint32_t timeout)
{
    UINT tx_status = tx_mutex_get(&canbc_h->state_mutex, timeout);
    canbc_states_t* ret = NULL;

    if (tx_status == TX_SUCCESS)
    {
        ret = &canbc_h->states;
    }

    return ret;
}

/**
 * @brief       Releases CANBC states after editing
 *
 * @param[in]   canbc_h     CANBC handle
 */
void canbc_unlock_state(canbc_handle_t* canbc_h)
{
    tx_mutex_put(&canbc_h->state_mutex);
}