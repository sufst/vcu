#include "wheelspeed.h"

#include "log.h"
#include "main.h"
#include "rtos_utils.h"
#include <can_s.h>

static void wheelspeed_thread_entry(void *arg);
static void compute_and_broadcast(wheelspeed_context_t *wh);
static float compute_wheel_rpm(wheel_state_t *wheel, const config_wheelspeed_t *config);

status_t wheelspeed_init(wheelspeed_context_t *wh,
                         rtcan_handle_t *rtcan_h,
                         const config_wheelspeed_t *config_ptr)
{
    wh->config_ptr = config_ptr;
    wh->tx_error_log_tick = 0;
    wh->rtcan_h = rtcan_h;

    status_t status = STATUS_OK;

    wh->reading_mutex = xSemaphoreCreateMutexStatic(&wh->reading_mutex_cb);

    if (wh->reading_mutex == NULL)
    {
        status = STATUS_ERROR;
    }

    if (status == STATUS_OK)
    {
        status = rtos_task_create(&config_ptr->thread, wheelspeed_thread_entry, wh,
                                  &wh->thread);
    }

    return status;
}

/**
 * @brief       Wheelspeed thread
 *
 * @details     Runs every sample period (fixed rate, so the pulse count
 *              delta always covers exactly one sample period)
 */
static void wheelspeed_thread_entry(void *arg)
{
    wheelspeed_context_t *wh = (wheelspeed_context_t *)arg;
    TickType_t last_wake_time = xTaskGetTickCount();

    while (1)
    {
        (void)xTaskDelayUntil(&last_wake_time, wh->config_ptr->sample_period_ticks);
        compute_and_broadcast(wh);
    }
}

static float compute_wheel_rpm(wheel_state_t *wheel, const config_wheelspeed_t *config)
{
    uint32_t current = wheel->isr_count;
    uint32_t delta = current - wheel->prev_count;
    wheel->prev_count = current;

    float revs_per_second = (float)delta * configTICK_RATE_HZ /
        ((float)config->ticks_per_wheel * (float)config->sample_period_ticks);
    return revs_per_second * 60.0;
}

static void compute_and_broadcast(wheelspeed_context_t *wh)
{
    const config_wheelspeed_t *cfg = wh->config_ptr;

    float rpm_fr = compute_wheel_rpm(&wh->wheel_fr, cfg);
    float rpm_fl = compute_wheel_rpm(&wh->wheel_fl, cfg);
    float rpm_rr = compute_wheel_rpm(&wh->wheel_rr, cfg);
    float rpm_rl = compute_wheel_rpm(&wh->wheel_rl, cfg);

    float mps_fr = rpm_fr / 60.0 * cfg->wheel_circumference_meters;
    float mps_fl = rpm_fl / 60.0 * cfg->wheel_circumference_meters;
    float mps_rr = rpm_rr / 60.0 * cfg->wheel_circumference_meters;
    float mps_rl = rpm_rl / 60.0 * cfg->wheel_circumference_meters;

    if (xSemaphoreTake(wh->reading_mutex, 100) == pdPASS)
    {
        wh->last_reading.fl_mps = mps_fl;
        wh->last_reading.fr_mps = mps_fr;
        wh->last_reading.rl_mps = mps_rl;
        wh->last_reading.rr_mps = mps_rr;
        wh->last_reading.fl_rpm = rpm_fl;
        wh->last_reading.fr_rpm = rpm_fr;
        wh->last_reading.rl_rpm = rpm_rl;
        wh->last_reading.rr_rpm = rpm_rr;
        wh->last_reading.sample_count++;
        xSemaphoreGive(wh->reading_mutex);
    }
    else
    {
        LOG_ERROR("wheelspeed: reading locking error\n");
    }

    {
        struct can_s_wheel_rpm_t data = {
            .wheel_fr_rpm = can_s_wheel_rpm_wheel_fr_rpm_encode(rpm_fr),
            .wheel_fl_rpm = can_s_wheel_rpm_wheel_fl_rpm_encode(rpm_fl),
            .wheel_rr_rpm = can_s_wheel_rpm_wheel_rr_rpm_encode(rpm_rr),
            .wheel_rl_rpm = can_s_wheel_rpm_wheel_rl_rpm_encode(rpm_rl),
        };
        rtcan_msg_t msg = {
            .identifier = CAN_S_WHEEL_RPM_FRAME_ID,
            .length = CAN_S_WHEEL_RPM_LENGTH,
            .extended = CAN_S_WHEEL_RPM_IS_EXTENDED,
        };
        can_s_wheel_rpm_pack(msg.data, &data, msg.length);
        if (rtcan_transmit(wh->rtcan_h, &msg) != RTCAN_OK &&
            rtos_every(&wh->tx_error_log_tick, configTICK_RATE_HZ))
        {
            LOG_ERROR("wheelspeed: failed to transmit RPM\n");
        }
    }

    {
        struct can_s_wheel_speeds_t data = {
            .wheel_fr_speed = can_s_wheel_speeds_wheel_fr_speed_encode(mps_fr),
            .wheel_fl_speed = can_s_wheel_speeds_wheel_fl_speed_encode(mps_fl),
            .wheel_rr_speed = can_s_wheel_speeds_wheel_rr_speed_encode(mps_rr),
            .wheel_rl_speed = can_s_wheel_speeds_wheel_rl_speed_encode(mps_rl),
        };
        rtcan_msg_t msg = {
            .identifier = CAN_S_WHEEL_SPEEDS_FRAME_ID,
            .length = CAN_S_WHEEL_SPEEDS_LENGTH,
            .extended = CAN_S_WHEEL_SPEEDS_IS_EXTENDED,
        };
        can_s_wheel_speeds_pack(msg.data, &data, msg.length);
        if (rtcan_transmit(wh->rtcan_h, &msg) != RTCAN_OK &&
            rtos_every(&wh->tx_error_log_tick, configTICK_RATE_HZ))
        {
            LOG_ERROR("wheelspeed: failed to transmit speeds\n");
        }
    }
}

void wheelspeed_handle_exti(wheelspeed_context_t *wh, uint16_t gpio_pin)
{
    if (gpio_pin == WHEELSPEED_FR_Pin)
        wh->wheel_fr.isr_count++;
    else if (gpio_pin == WHEELSPEED_FL_Pin)
        wh->wheel_fl.isr_count++;
    else if (gpio_pin == WHEELSPEED_RR_Pin)
        wh->wheel_rr.isr_count++;
    else if (gpio_pin == WHEELSPEED_RL_Pin)
        wh->wheel_rl.isr_count++;
}

status_t wheelspeed_get_speeds(wheelspeed_context_t *wh, wheelspeed_reading_t *result)
{
    status_t status = STATUS_ERROR;

    if (xSemaphoreTake(wh->reading_mutex, 100) == pdPASS)
    {
        *result = wh->last_reading;
        status = STATUS_OK;
        xSemaphoreGive(wh->reading_mutex);
    }
    else
    {
        LOG_ERROR("wheelspeed: reading locking error\n");
    }

    return status;
}
