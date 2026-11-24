#include "fans.h"

#include "log.h"
#include "main.h"
#include "rtos_utils.h"
#include <can_s.h>

static void fans_thread_entry(void *arg);

status_t fans_init(fans_context_t *fh,
                   rtcan_handle_t *rtcan_s_h,
                   const config_fans_t *config_ptr)
{
    // always initialise the state, the control service uses it even when the
    // fans service is disabled
    fh->config_ptr = config_ptr;
    fh->rtcan_s_h = rtcan_s_h;
    fh->fan_switch_status = false;
    fh->fan_thermal_pwr = false;

    if (!config_ptr->enable)
        return STATUS_OK;

    status_t status = STATUS_OK;

    // create CAN receive queue
    if (rtcan_os_queue_create(&fh->can_rx_queue, NULL, sizeof(rtcan_msg_t *),
                              FANS_RX_QUEUE_SIZE, fh->can_rx_queue_mem,
                              sizeof(fh->can_rx_queue_mem)) != RTCAN_OS_OK)
    {
        status = STATUS_ERROR;
    }

    // create thread
    if (status == STATUS_OK)
    {
        status = rtos_task_create(&config_ptr->thread, fans_thread_entry, fh, &fh->thread);
    }

    return status;
}

static void process_broadcast(fans_context_t *fh, const rtcan_msg_t *msg_ptr)
{
    if (msg_ptr->identifier != CAN_S_DASH_SENSORS_DIGITAL_FRAME_ID)
        return;
    struct can_s_dash_sensors_digital_t data;
    can_s_dash_sensors_digital_unpack(&data, msg_ptr->data, msg_ptr->length);
    fh->fan_switch_status = fh->config_ptr->inverted ? !data.fan_switch : data.fan_switch;
}

static void fans_thread_entry(void *arg)
{
    fans_context_t *fh = (fans_context_t *)arg;

    rtcan_status_t status =
        rtcan_subscribe(fh->rtcan_s_h, CAN_S_DASH_SENSORS_DIGITAL_FRAME_ID, fh->can_rx_queue);

    if (status != RTCAN_OK)
    {
        LOG_ERROR("Could not subscribe on message. Terminating thread\n");
        vTaskDelete(NULL);
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
