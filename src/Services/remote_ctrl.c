/*****************************************************************************
 * @file    remote_ctrl.c
 * @author  Dmytro Avdieienko (@Avdieienko, da3e22@soton.ac.uk)
 * @brief   Remote control service
 * @details Thread-safe remote control service implementation intended for use on the dyno
 ****************************************************************************/

#include "remote_ctrl.h"

#include "rtos_utils.h"

static void remote_ctrl_thread_entry(void *arg);
static status_t lock_sim_sensors(remote_ctrl_context_t *remote_ctrl_ptr, TickType_t timeout);
static void unlock_sim_sensors(remote_ctrl_context_t *remote_ctrl_ptr);
static void reset_remote_ctrl_requests(remote_ctrl_context_t *remote_ctrl_ptr);

static void process_broadcast(remote_ctrl_context_t *remote_ctrl_ptr,
                              const rtcan_msg_t *msg_ptr);

status_t remote_ctrl_init(remote_ctrl_context_t *remote_ctrl_ptr,
                          canbc_context_t *canbc_ptr,
                          rtcan_handle_t *rtcan_s_prt,
                          const config_remote_ctrl_t *config_ptr)
{
    remote_ctrl_ptr->config_ptr = config_ptr;
    remote_ctrl_ptr->rtcan_s_ptr = rtcan_s_prt;
    remote_ctrl_ptr->canbc_ptr = canbc_ptr;
    remote_ctrl_ptr->requests = (struct can_s_vcu_simulation_t){ 0 };
    remote_ctrl_ptr->ts_on_prev = false;
    remote_ctrl_ptr->r2d_prev = false;
    remote_ctrl_ptr->timed_out = false;

    status_t status = STATUS_OK;

    // create CAN receive queue
    if (rtcan_os_queue_create(&remote_ctrl_ptr->can_rx_queue, NULL, sizeof(rtcan_msg_t *),
                              REMOTE_CTRL_RX_QUEUE_SIZE, remote_ctrl_ptr->can_rx_queue_mem,
                              sizeof(remote_ctrl_ptr->can_rx_queue_mem)) != RTCAN_OS_OK)
    {
        status = STATUS_ERROR;
    }

    // create state mutex
    if (status == STATUS_OK)
    {
        remote_ctrl_ptr->sensor_mutex =
            xSemaphoreCreateMutexStatic(&remote_ctrl_ptr->sensor_mutex_cb);

        if (remote_ctrl_ptr->sensor_mutex == NULL)
        {
            status = STATUS_ERROR;
        }
    }

    // create service thread (only once everything it uses exists)
    if (status == STATUS_OK)
    {
        status = rtos_task_create(&config_ptr->thread, remote_ctrl_thread_entry,
                                  remote_ctrl_ptr, &remote_ctrl_ptr->thread);
    }

    return status;
}

static void remote_ctrl_thread_entry(void *arg)
{
    remote_ctrl_context_t *remote_ctrl_ptr = (remote_ctrl_context_t *)arg;
    const config_remote_ctrl_t *config_ptr = remote_ctrl_ptr->config_ptr;

    uint32_t rtcan_channel = CAN_S_VCU_SIMULATION_FRAME_ID;

    rtcan_status_t status = rtcan_subscribe(remote_ctrl_ptr->rtcan_s_ptr, rtcan_channel,
                                            remote_ctrl_ptr->can_rx_queue);

    if (status != RTCAN_OK)
    {
        LOG_ERROR("Failed to subscribe to the CAN_S_VCU_SIMULATION message");
        vTaskDelete(NULL);
    }

    while (1)
    {
        rtcan_msg_t *msg_ptr = NULL;
        rtcan_osal_status_t rec_status =
            rtcan_os_queue_receive(remote_ctrl_ptr->can_rx_queue, &msg_ptr,
                                   config_ptr->broadcast_timeout_ticks);

        if (rec_status == RTCAN_OS_OK && msg_ptr != NULL)
        {
            remote_ctrl_ptr->timed_out = false;

            if (lock_sim_sensors(remote_ctrl_ptr, 100) == STATUS_OK)
            {
                process_broadcast(remote_ctrl_ptr, msg_ptr);
                unlock_sim_sensors(remote_ctrl_ptr);
            }
            else
            {
                LOG_ERROR("Error locking sensors\n");
            }

            // always release the message, or the RTCAN receive pool leaks
            rtcan_msg_consumed(remote_ctrl_ptr->rtcan_s_ptr, msg_ptr);
        }
        else if (rec_status == RTCAN_OS_TIMEOUT)
        {
            // the remote control is normally not connected, so only log
            // when the link is lost, not every timeout
            if (!remote_ctrl_ptr->timed_out)
            {
                LOG_WARN("Remote control: broadcast timeout\n");
                remote_ctrl_ptr->timed_out = true;
            }
            reset_remote_ctrl_requests(remote_ctrl_ptr);
        }
        else
        {
            LOG_ERROR("Broadcast Error: %d\n", rec_status);
            reset_remote_ctrl_requests(remote_ctrl_ptr);
        }
        vTaskDelay(config_ptr->period);
    }
}

static status_t lock_sim_sensors(remote_ctrl_context_t *remote_ctrl_ptr, TickType_t timeout)
{
    if (xSemaphoreTake(remote_ctrl_ptr->sensor_mutex, timeout) == pdPASS)
    {
        return STATUS_OK;
    }
    return STATUS_ERROR;
}

static void unlock_sim_sensors(remote_ctrl_context_t *remote_ctrl_ptr)
{
    xSemaphoreGive(remote_ctrl_ptr->sensor_mutex);
}

uint16_t remote_get_torque_reading(remote_ctrl_context_t *remote_ctrl_ptr)
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

uint8_t remote_get_ts_on_pressed(remote_ctrl_context_t *remote_ctrl_ptr)
{
    uint8_t result = 0u;

    if (lock_sim_sensors(remote_ctrl_ptr, 100) == STATUS_OK)
    {
        bool state = remote_ctrl_ptr->requests.sim_ts_on;
        result = state && !remote_ctrl_ptr->ts_on_prev;
        remote_ctrl_ptr->ts_on_prev = state;
        unlock_sim_sensors(remote_ctrl_ptr);
    }
    else
    {
        LOG_ERROR("TS locking error\n");
    }

    return result;
}

uint8_t remote_get_r2d_pressed(remote_ctrl_context_t *remote_ctrl_ptr)
{
    uint8_t result = 0u;

    if (lock_sim_sensors(remote_ctrl_ptr, 100) == STATUS_OK)
    {
        bool state = remote_ctrl_ptr->requests.sim_r2_d;
        if (!can_t_vcu_state_vcu_r2_d_is_in_range(state))
        {
            LOG_WARN("R2D requested is over the limit, setting it to 0\n");
            state = false;
        }
        result = state && !remote_ctrl_ptr->r2d_prev;
        remote_ctrl_ptr->r2d_prev = state;
        unlock_sim_sensors(remote_ctrl_ptr);
    }
    else
    {
        LOG_ERROR("R2D locking error\n");
    }

    return result;
}

static void process_broadcast(remote_ctrl_context_t *remote_ctrl_ptr, const rtcan_msg_t *msg_ptr)
{
    switch (msg_ptr->identifier)
    {
    case CAN_S_VCU_SIMULATION_FRAME_ID:
    {
        can_s_vcu_simulation_unpack(&remote_ctrl_ptr->requests, msg_ptr->data,
                                    msg_ptr->length);
        break;
    }
    default: break;
    }
}

static void reset_remote_ctrl_requests(remote_ctrl_context_t *remote_ctrl_ptr)
{
    if (lock_sim_sensors(remote_ctrl_ptr, 100) == STATUS_OK)
    {
        remote_ctrl_ptr->requests = (struct can_s_vcu_simulation_t){ 0 };
        remote_ctrl_ptr->ts_on_prev = false;
        remote_ctrl_ptr->r2d_prev = false;
        unlock_sim_sensors(remote_ctrl_ptr);
    }
    else
    {
        LOG_ERROR("Error locking sensors\n");
    }
}