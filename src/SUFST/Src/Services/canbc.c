#include "canbc.h"

#include <can_s.h>
#include <can_t.h>

/*
 * internal function prototypes
 */
static void canbc_thread_entry(ULONG input);
static void sleep_till_next_bc(canbc_context_t *canbc_h, uint32_t start_time);
static void send_bc_messages(canbc_context_t *canbc_h);

/**
 * @brief       Initialise CANBC service
 *
 * @param[in]   canbc_h         CANBC handle
 * @param[in]   rtcan_t_h       RTCAN handle for the tractive bus
 * @param[in]   rtcan_s_h       RTCAN handle for the sensor bus
 * @param[in]   stack_pool_ptr  Application memory pool
 * @param[in]   config_ptr      Configuration
 */
status_t canbc_init(canbc_context_t *canbc_h,
                    rtcan_handle_t *rtcan_t_h,
                    rtcan_handle_t *rtcan_s_h,
                    TX_BYTE_POOL *stack_pool_ptr,
                    const config_canbc_t *config_ptr)
{
    canbc_h->rtcan_t_h = rtcan_t_h;
    canbc_h->rtcan_s_h = rtcan_s_h;
    canbc_h->config_ptr = config_ptr;
    canbc_h->rolling_counter = 0;
    canbc_h->rtcan1_error = 0;
    canbc_h->rtcan2_error = 0;
    canbc_h->canbc_error = 0;

    // create service thread
    void *stack_ptr = NULL;
    UINT tx_status = tx_byte_allocate(stack_pool_ptr, &stack_ptr,
                                      config_ptr->thread.stack_size, TX_NO_WAIT);

    if (tx_status == TX_SUCCESS)
    {
        tx_status =
            tx_thread_create(&canbc_h->thread, (CHAR *)config_ptr->thread.name,
                             canbc_thread_entry, (ULONG)canbc_h, stack_ptr,
                             config_ptr->thread.stack_size, config_ptr->thread.priority,
                             config_ptr->thread.priority, TX_NO_TIME_SLICE, TX_AUTO_START);
    }

    // create state mutex
    if (tx_status == TX_SUCCESS)
    {
        tx_status = tx_mutex_create(&canbc_h->state_mutex, NULL, TX_INHERIT);
    }

    return (tx_status == TX_SUCCESS) ? STATUS_OK : STATUS_ERROR;
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
    canbc_context_t *canbc_h = (canbc_context_t *)input;

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
 * @details     The broadcast states packed into the CAN messages using the
 *              packing functions generated from the DBC (see `can-defs` repo).
 *
 * @note        This approach doesn't scale particularly well, but at the
 *              moment there are a limited number of broadcast states so this
 *              implementation is the simplest.
 *
 * @param[in]   canbc_h     CANBC handle
 */
static void send_bc_messages(canbc_context_t *canbc_h)
{
    UINT tx_status = tx_mutex_get(&canbc_h->state_mutex, TX_WAIT_FOREVER);

    if (tx_status != TX_SUCCESS)
    {
        canbc_h->canbc_error = 1;
        LOG_ERROR("canbc: failed to lock state\n");
        return;
    }

    canbc_h->states.state.vcu_rolling_counter++;
    canbc_states_t snapshot = canbc_h->states;
    tx_mutex_put(&canbc_h->state_mutex);

    // states
    {
        rtcan_msg_t message = { .identifier = CAN_T_VCU_STATE_FRAME_ID,
                                .length = CAN_T_VCU_STATE_LENGTH,
                                .extended = CAN_T_VCU_STATE_IS_EXTENDED };

        can_t_vcu_state_pack(message.data, &snapshot.state, message.length);
        if (rtcan_transmit(canbc_h->rtcan_t_h, &message) != RTCAN_OK)
        {
            canbc_h->rtcan2_error = 1;
            LOG_ERROR("canbc: failed to transmit VCU state\n");
        }
    }

    // sensors
    {
        rtcan_msg_t message = { .identifier = CAN_S_VCU_SENSORS_FRAME_ID,
                                .length = CAN_S_VCU_SENSORS_LENGTH,
                                .extended = CAN_S_VCU_SENSORS_IS_EXTENDED };

        can_s_vcu_sensors_pack(message.data, &snapshot.sensors, message.length);
        if (rtcan_transmit(canbc_h->rtcan_s_h, &message) != RTCAN_OK)
        {
            canbc_h->rtcan1_error = 1;
            LOG_ERROR("canbc: failed to transmit VCU sensors\n");
        }
    }

    // temps
    {
        rtcan_msg_t message = { .identifier = CAN_S_VCU_TEMPS_FRAME_ID,
                                .length = CAN_S_VCU_TEMPS_LENGTH,
                                .extended = CAN_S_VCU_TEMPS_IS_EXTENDED };

        can_s_vcu_temps_pack(message.data, &snapshot.temps, message.length);
        if (rtcan_transmit(canbc_h->rtcan_s_h, &message) != RTCAN_OK)
        {
            canbc_h->rtcan1_error = 1;
            LOG_ERROR("canbc: failed to transmit VCU temps\n");
        }
    }

    // errors
    {
        rtcan_msg_t message = { .identifier = CAN_T_VCU_ERROR_FRAME_ID,
                                .length = CAN_T_VCU_ERROR_LENGTH,
                                .extended = CAN_T_VCU_ERROR_IS_EXTENDED };

        snapshot.errors.vcu_rtcan1_error = canbc_h->rtcan1_error;
        snapshot.errors.vcu_rtcan2_error = canbc_h->rtcan2_error;
        snapshot.errors.vcu_canbc_error = canbc_h->canbc_error;

        can_t_vcu_error_pack(message.data, &snapshot.errors, message.length);
        if (rtcan_transmit(canbc_h->rtcan_t_h, &message) != RTCAN_OK)
        {
            canbc_h->rtcan2_error = 1;
            LOG_ERROR("canbc: failed to transmit VCU errors\n");
        }
    }

    // pdm
    {
        rtcan_msg_t message = { .identifier = CAN_T_VCU_PDM_FRAME_ID,
                                .length = CAN_T_VCU_PDM_LENGTH,
                                .extended = CAN_T_VCU_PDM_IS_EXTENDED };

        can_t_vcu_pdm_pack(message.data, &snapshot.pdm, message.length);
        if (rtcan_transmit(canbc_h->rtcan_t_h, &message) != RTCAN_OK)
        {
            canbc_h->rtcan2_error = 1;
            LOG_ERROR("canbc: failed to transmit VCU PDM\n");
        }
    }

    // raw sensors debug
    {
        rtcan_msg_t message = { .identifier = CAN_S_VCU_SENSORS_RAW_FRAME_ID,
                                .length = CAN_S_VCU_SENSORS_RAW_LENGTH,
                                .extended = CAN_S_VCU_SENSORS_RAW_IS_EXTENDED };

        can_s_vcu_sensors_raw_pack(message.data, &snapshot.sensors_raw, message.length);
        if (rtcan_transmit(canbc_h->rtcan_s_h, &message) != RTCAN_OK)
        {
            canbc_h->rtcan1_error = 1;
            LOG_ERROR("canbc: failed to transmit VCU raw sensors\n");
        }
    }
}

/**
 * @brief       Suspends CANBC thread until the next broadcast is due
 *
 * @param[in]   canbc_h     CANBC handle
 * @param[in]   start_time  Timestamp at which last broadcast event started
 */
static void sleep_till_next_bc(canbc_context_t *canbc_h, uint32_t start_time)
{
    const uint32_t period = canbc_h->config_ptr->broadcast_period_ticks;
    const uint32_t run_time = tx_time_get() - start_time;
    uint32_t sleep_time = period - run_time;

    // check for overflow or zero, just in case
    if (run_time >= period)
    {
        sleep_time = (TX_TIMER_TICKS_PER_SECOND * 0.5);
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
canbc_states_t *canbc_lock_state(canbc_context_t *canbc_h, uint32_t timeout)
{
    UINT tx_status = tx_mutex_get(&canbc_h->state_mutex, timeout);
    canbc_states_t *ret = NULL;

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
void canbc_unlock_state(canbc_context_t *canbc_h)
{
    tx_mutex_put(&canbc_h->state_mutex);
}
