#include "fans.h"

#include "log.h"
#include "main.h"
#include <can_s.h>

static void fans_thread_entry(ULONG input);

status_t fans_init(fans_context_t *fh,
                   rtcan_handle_t *rtcan_s_h,
                   TX_BYTE_POOL *stack_pool_ptr,
                   const config_fans_t *config_ptr)
{
    if (!config_ptr->enable)
        return STATUS_OK;

    fh->config_ptr = config_ptr;
    fh->rtcan_s_h = rtcan_s_h;
    fh->fan_switch_status = false;
    fh->fan_thermal_pwr = false;

    void *stack_ptr = NULL;
    UINT tx_status = tx_byte_allocate(stack_pool_ptr, &stack_ptr,
                                      config_ptr->thread.stack_size, TX_NO_WAIT);

    // create CAN receive queue
    if (tx_status == TX_SUCCESS)
    {
        if (rtcan_os_queue_create(&fh->can_rx_queue, NULL, sizeof(rtcan_msg_t *),
                                  FANS_RX_QUEUE_SIZE, fh->can_rx_queue_mem,
                                  sizeof(fh->can_rx_queue_mem)) != RTCAN_OS_OK)
        {
            tx_status = TX_START_ERROR;
        }
    }

    // create thread
    if (tx_status == TX_SUCCESS)
    {
        tx_status =
            tx_thread_create(&fh->thread, (CHAR *)config_ptr->thread.name, fans_thread_entry,
                             (ULONG)fh, stack_ptr, config_ptr->thread.stack_size,
                             config_ptr->thread.priority, config_ptr->thread.priority,
                             TX_NO_TIME_SLICE, TX_AUTO_START);
    }

    return (tx_status == TX_SUCCESS) ? STATUS_OK : STATUS_ERROR;
}

static void process_broadcast(fans_context_t *fh, const rtcan_msg_t *msg_ptr)
{
    if (msg_ptr->identifier != CAN_S_DASH_SENSORS_DIGITAL_FRAME_ID)
        return;
    struct can_s_dash_sensors_digital_t data;
    can_s_dash_sensors_digital_unpack(&data, msg_ptr->data, msg_ptr->length);
    fh->fan_switch_status = fh->config_ptr->inverted ? !data.fan_switch : data.fan_switch;
}

static void fans_thread_entry(ULONG input)
{
    fans_context_t *fh = (fans_context_t *)input;

    rtcan_status_t status =
        rtcan_subscribe(fh->rtcan_s_h, CAN_S_DASH_SENSORS_DIGITAL_FRAME_ID, fh->can_rx_queue);

    if (status != RTCAN_OK)
    {
        LOG_ERROR("Could not subscribe on message. Terminating thread\n");
        tx_thread_terminate(&fh->thread);
    }

    while (1)
    {
        rtcan_msg_t *msg_ptr = NULL;
        rtcan_osal_status_t rec_status =
            rtcan_os_queue_receive(fh->can_rx_queue, &msg_ptr,
                                   fh->config_ptr->broadcast_timeout_ticks);

        if (rec_status == RTCAN_OS_OK && msg_ptr != NULL)
        {
            process_broadcast(fh, msg_ptr);
            rtcan_msg_consumed(fh->rtcan_s_h, msg_ptr);
            continue;
        }

        LOG_INFO("FAN switch broadcast timeout\n");
    }
}

/**
 * @brief       Updates the thermal hysteresis state from a temperature reading
 *
 * @param[in]   fh          Fans context
 * @param[in]   max_temp    Highest of the motor / inverter temperatures
 */
void fans_update_thermal(fans_context_t *fh, int16_t max_temp)
{
    if (max_temp > fh->config_ptr->fan_on_threshold)
    {
        fh->fan_thermal_pwr = true;
    }
    else if (fh->fan_thermal_pwr && max_temp < fh->config_ptr->fan_off_threshold)
    {
        fh->fan_thermal_pwr = false;
    }
}

/**
 * @brief       Whether the fan should be powered, combining the thermal
 *              hysteresis output with the driver's manual override switch
 *
 * @param[in]   fh  Fans context
 */
bool fans_output_pwr(const fans_context_t *fh)
{
    return fh->fan_thermal_pwr || fh->fan_switch_status;
}
