/*****************************************************************************
 * @file    remote_ctrl.c
 * @author  Dmytro Avdieienko (@Avdieienko, da3e22@soton.ac.uk)
 * @brief   Remote control service
 * @details Thread-safe remote control service implementation intended for use
 *on the dyno
 ****************************************************************************/

#include "remote_ctrl.h"

#include "clip_to_range.h"

static void remote_ctrl_thread_entry(ULONG input);
static status_t lock_sim_sensors(remote_ctrl_context_t* remote_ctrl_ptr,
                                 uint32_t timeout);
static void unlock_sim_sensors(remote_ctrl_context_t* remote_ctrl_ptr);
static void process_broadcast(remote_ctrl_context_t* remote_ctrl_ptr,
                              const rtcan_msg_t* msg_ptr);
void reset_remote_ctrl_requests(remote_ctrl_context_t* remote_ctrl_ptr);

#define BPS_LIGHT_THRESH 5

status_t remote_ctrl_init(remote_ctrl_context_t* remote_ctrl_ptr,
                          canbc_context_t* canbc_ptr,
                          TX_BYTE_POOL* stack_pool_ptr,
                          rtcan_handle_t* rtcan_s_prt,
                          const config_remote_ctrl_t* config_ptr)
{
    remote_ctrl_ptr->config_ptr = config_ptr;
    remote_ctrl_ptr->rtcan_s_ptr = rtcan_s_prt;
    remote_ctrl_ptr->canbc_ptr = canbc_ptr;
    can_s_vcu_simulate_init(&remote_ctrl_ptr->requests);

    status_t status = STATUS_OK;

    // create service thread
    void* stack_ptr = NULL;
    UINT tx_status = tx_byte_allocate(stack_pool_ptr,
                                      &stack_ptr,
                                      config_ptr->thread.stack_size,
                                      TX_NO_WAIT);

    if (tx_status == TX_SUCCESS)
    {
        tx_status = tx_thread_create(&remote_ctrl_ptr->thread,
                                     (CHAR*) config_ptr->thread.name,
                                     remote_ctrl_thread_entry,
                                     (ULONG) remote_ctrl_ptr,
                                     stack_ptr,
                                     config_ptr->thread.stack_size,
                                     config_ptr->thread.priority,
                                     config_ptr->thread.priority,
                                     TX_NO_TIME_SLICE,
                                     TX_AUTO_START);
    }

    // create CAN receive queue
    if (tx_status == TX_SUCCESS)
    {
        tx_status = tx_queue_create(&remote_ctrl_ptr->can_rx_queue,
                                    NULL,
                                    TX_1_ULONG,
                                    remote_ctrl_ptr->can_rx_queue_mem,
                                    sizeof(remote_ctrl_ptr->can_rx_queue_mem));
    }

    // create state mutex
    if (tx_status == TX_SUCCESS)
    {
        tx_status
            = tx_mutex_create(&remote_ctrl_ptr->sensor_mutex, NULL, TX_INHERIT);
    }

    if (tx_status != TX_SUCCESS)
    {
        status = STATUS_ERROR;
    }

    // check all ok
    if (status != STATUS_OK)
    {
        tx_thread_terminate(&remote_ctrl_ptr->thread);
    }

    return status;
}

static void remote_ctrl_thread_entry(ULONG input)
{

#ifndef VCU_SIMULATION_MODE
#else
    remote_ctrl_context_t* remote_ctrl_ptr = (remote_ctrl_context_t*) input;
    const config_remote_ctrl_t* config_ptr = remote_ctrl_ptr->config_ptr;

    uint32_t rtcan_channel = CAN_S_VCU_SIMULATION_FRAME_ID;

    rtcan_status_t status = rtcan_subscribe(remote_ctrl_ptr->rtcan_s_ptr,
                                            rtcan_channel,
                                            &remote_ctrl_ptr->can_rx_queue);

    if (status != RTCAN_OK)
    {
        LOG_ERROR("Failed to subscribe to the CAN_S_VCU_SIMULATION message");
        tx_thread_terminate(&remote_ctrl_ptr->thread);
    }

    while (1)
    {
        rtcan_msg_t* msg_ptr = NULL;
        UINT status = tx_queue_receive(&remote_ctrl_ptr->can_rx_queue,
                                       &msg_ptr,
                                       config_ptr->broadcast_timeout_ticks);

        if (status == TX_SUCCESS && msg_ptr != NULL)
        {
            if (lock_sim_sensors(remote_ctrl_ptr, 100) == STATUS_OK)
            {
                process_broadcast(remote_ctrl_ptr, msg_ptr);
                rtcan_msg_consumed(remote_ctrl_ptr->rtcan_s_ptr, msg_ptr);
                remote_ctrl_ptr->brakelight_pwr
                    = (remote_ctrl_ptr->requests.sim_bps > BPS_LIGHT_THRESH);
                unlock_sim_sensors(remote_ctrl_ptr);
            }
            else
            {
                LOG_ERROR("Error locking sensors\n");
            }
        }
        else if (status != TX_SUCCESS && msg_ptr == NULL)
        {
            if (status == TX_QUEUE_EMPTY)
            {
                LOG_ERROR("TX Broadcast Error & Msg_ptr is null\n");
            }
            else
            {
                LOG_ERROR("TX Error & Msg_ptr is null\n");
            }
            reset_remote_ctrl_requests(remote_ctrl_ptr);
        }
        else if (status == TX_QUEUE_EMPTY)
        {
            LOG_ERROR("Broadcast timeout\n");
            reset_remote_ctrl_requests(remote_ctrl_ptr);
        }
        else
        {
            LOG_ERROR("Broadcast Error\n");
            reset_remote_ctrl_requests(remote_ctrl_ptr);
        }
        remote_ctrl_update_canbc_states(remote_ctrl_ptr);
        tx_thread_sleep(config_ptr->period);
    }
#endif
}

static status_t lock_sim_sensors(remote_ctrl_context_t* remote_ctrl_ptr,
                                 uint32_t timeout)
{
    UINT tx_status = tx_mutex_get(&remote_ctrl_ptr->sensor_mutex, timeout);

    if (tx_status == TX_SUCCESS)
    {
        return STATUS_OK;
    }
    return STATUS_ERROR;
}

static void unlock_sim_sensors(remote_ctrl_context_t* remote_ctrl_ptr)
{
    tx_mutex_put(&remote_ctrl_ptr->sensor_mutex);
}

uint16_t remote_get_torque_reading(remote_ctrl_context_t* remote_ctrl_ptr)
{
    uint16_t result = 0; // Set initial torque to 0

    if (lock_sim_sensors(remote_ctrl_ptr, 100) == STATUS_OK)
    {
        result = remote_ctrl_ptr->requests.sim_torque_request;
        if (result > remote_ctrl_ptr->config_ptr->torque_limit)
        {
            LOG_WARN("Torque requested is over the limit, setting it to 0\n");
            result = 0;
        }
        unlock_sim_sensors(remote_ctrl_ptr);
    }
    else
    {
        LOG_ERROR("Torque locking error\n");
    }

    return result;
}

status_t remote_get_bps_reading(remote_ctrl_context_t* remote_ctrl_ptr,
                                uint16_t* result)
{
    status_t status = STATUS_ERROR;

    if (lock_sim_sensors(remote_ctrl_ptr, 100) == STATUS_OK)
    {
        *result = remote_ctrl_ptr->requests.sim_bps;
        status = STATUS_OK;
        unlock_sim_sensors(remote_ctrl_ptr);
    }
    else
    {
        LOG_ERROR("BPS locking error\n");
    }

    return status;
}

status_t remote_get_apps_reading(remote_ctrl_context_t* remote_ctrl_ptr,
                                 uint16_t* result)
{
    status_t status = STATUS_ERROR;

    if (lock_sim_sensors(remote_ctrl_ptr, 100) == STATUS_OK)
    {
        *result = remote_ctrl_ptr->requests.sim_apps;
        status = STATUS_OK;
        unlock_sim_sensors(remote_ctrl_ptr);
    }
    else
    {
        LOG_ERROR("APPS locking error\n");
    }

    return status;
}

uint8_t remote_get_ts_on_reading(remote_ctrl_context_t* remote_ctrl_ptr)
{
    uint8_t result = 0u;

    if (lock_sim_sensors(remote_ctrl_ptr, 100) == STATUS_OK)
    {
        result = remote_ctrl_ptr->requests.sim_ts_on;
        unlock_sim_sensors(remote_ctrl_ptr);
    }
    else
    {
        LOG_ERROR("TS locking error\n");
    }

    return result;
}

uint8_t remote_get_r2d_reading(remote_ctrl_context_t* remote_ctrl_ptr)
{
    uint8_t result = 0u;

    if (lock_sim_sensors(remote_ctrl_ptr, 100) == STATUS_OK)
    {
        result = remote_ctrl_ptr->requests.sim_r2_d;
        if (!can_s_vcu_state_vcu_r2_d_is_in_range(result))
        {
            LOG_WARN("R2D requested is over the limit, setting it to 0\n");
            result = 0u;
        }
        unlock_sim_sensors(remote_ctrl_ptr);
    }
    else
    {
        LOG_ERROR("R2D locking error\n");
    }

    return result;
}

void remote_ctrl_update_canbc_states(remote_ctrl_context_t* remote_ctrl_ptr)
{
    canbc_states_t* states
        = canbc_lock_state(remote_ctrl_ptr->canbc_ptr, TX_NO_WAIT);

    if (states != NULL)
    {
        states->pdm.brakelight = remote_ctrl_ptr->brakelight_pwr;
        states->sensors.vcu_apps = remote_ctrl_ptr->requests.sim_apps;
        states->sensors.vcu_bps = remote_ctrl_ptr->requests.sim_bps;
        canbc_unlock_state(remote_ctrl_ptr->canbc_ptr);
    }
    else
    {
        LOG_ERROR("CANBC locking failure\n");
    }
}

void process_broadcast(remote_ctrl_context_t* remote_ctrl_ptr,
                       const rtcan_msg_t* msg_ptr)
{
    switch (msg_ptr->identifier)
    {
    case CAN_S_VCU_SIMULATION_FRAME_ID:
    {
        can_s_vcu_simulation_unpack(&remote_ctrl_ptr->requests,
                                    msg_ptr->data,
                                    msg_ptr->length);
        break;
    }
    default:
        break;
    }
}

void reset_remote_ctrl_requests(remote_ctrl_context_t* remote_ctrl_ptr)
{
    if (lock_sim_sensors(remote_ctrl_ptr, 100) == STATUS_OK)
    {
        can_s_vcu_simulate_init(&remote_ctrl_ptr->requests);
        unlock_sim_sensors(remote_ctrl_ptr);
    }
    else
    {
        LOG_ERROR("Error locking sensors\n");
    }
}

uint16_t remote_get_power_reading(remote_ctrl_context_t* remote_ctrl_ptr)
{
    uint16_t result = 0; // Set initial power to 0

    if (lock_sim_sensors(remote_ctrl_ptr, 100) == STATUS_OK)
    {
        result = remote_ctrl_ptr->requests.sim_power;
        unlock_sim_sensors(remote_ctrl_ptr);
    }
    else
    {
        LOG_ERROR("Power locking error\n");
    }

    return result;
}