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

    /* free running cycle counter used to timestamp wheel edges */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->LAR = 0xC5ACCE55U; // unlock (required on Cortex-M7)
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

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

/**
 * @brief       Wheel speed from edge timestamps
 *
 * @details     Counting pulses in a fixed window quantises the speed (one pulse
 *              is ~5 km/h in 20 ms) and is wrong if the thread runs late. Instead
 *              the ISR timestamps each edge with the DWT cycle counter and the
 *              speed is whole pulses divided by the time between the first and
 *              last edge since the previous reading, so the sample window length
 *              no longer matters. With no new edge the reading can only fall
 *              (at most one pulse per time since the last edge) and reads 0 once
 *              the wheel has been quiet for stopped_timeout_s.
 */
static float compute_wheel_rpm(wheel_state_t *wheel, const config_wheelspeed_t *config)
{
    uint32_t count, edge;
    for (int i = 0; i < 3; i++) // ISR updates edge then count: retry if it ran mid-read
    {
        count = wheel->isr_count;
        edge = wheel->last_edge;
        if (count == wheel->isr_count)
            break;
    }

    const float cycles_per_rev = (float)SystemCoreClock * 60.0f / (float)config->ticks_per_wheel;
    uint32_t delta = count - wheel->ref_count;

    if (delta > 0 && wheel->ref_valid)
    {
        uint32_t dt = edge - wheel->ref_edge;
        float rpm = (dt > 0) ? (float)delta * cycles_per_rev / (float)dt : config->max_wheel_rpm + 1.0f;
        wheel->ref_count = count;
        wheel->ref_edge = edge;
        if (rpm <= config->max_wheel_rpm) // reject implausible readings, keep the last good one
            wheel->rpm = rpm;
    }
    else if (delta > 0) // first edge since standstill: nothing to time against yet
    {
        wheel->ref_count = count;
        wheel->ref_edge = edge;
        wheel->ref_valid = true;
        wheel->rpm = 0.0f;
    }
    else if (wheel->ref_valid)
    {
        uint32_t quiet = DWT->CYCCNT - edge;
        if ((float)quiet > config->stopped_timeout_s * (float)SystemCoreClock)
        {
            wheel->ref_valid = false;
            wheel->rpm = 0.0f;
        }
        else
        {
            float bound = (quiet > 0) ? cycles_per_rev / (float)quiet : wheel->rpm;
            if (bound < wheel->rpm)
                wheel->rpm = bound;
        }
    }
    else
    {
        wheel->rpm = 0.0f;
    }

    return wheel->rpm;
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
    {
        wh->wheel_fr.last_edge = DWT->CYCCNT;
        wh->wheel_fr.isr_count++;
    }
    else if (gpio_pin == WHEELSPEED_FL_Pin)
    {
        wh->wheel_fl.last_edge = DWT->CYCCNT;
        wh->wheel_fl.isr_count++;
    }
    else if (gpio_pin == WHEELSPEED_RR_Pin)
    {
        wh->wheel_rr.last_edge = DWT->CYCCNT;
        wh->wheel_rr.isr_count++;
    }
    else if (gpio_pin == WHEELSPEED_RL_Pin)
    {
        wh->wheel_rl.last_edge = DWT->CYCCNT;
        wh->wheel_rl.isr_count++;
    }
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
