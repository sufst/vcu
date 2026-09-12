#include <math.h>

#include "torque_limiters.h"

void torque_limiters_init(torque_limiters_context_t *ctx,
                          const config_torque_limiters_t *config_ptr)
{
    ctx->config_ptr = config_ptr;
    ctx->min_denominator_rpm =
        fmaxf(config_ptr->min_denominator_rpm,
              config_ptr->min_motor_speed_rpm / config_ptr->gear_ratio);
    compressor_init(&ctx->slip, &config_ptr->slip);
    compressor_init(&ctx->power, &config_ptr->power);
}

uint16_t torque_limiters_apply(torque_limiters_context_t *ctx,
                               uint16_t torque_request,
                               bool slip_enabled,
                               int16_t motor_rpm,
                               float front_wheel_rpm,
                               float electrical_power_w)
{
    bool front_reliable = front_wheel_rpm >= ctx->min_denominator_rpm;
    bool slip_active = slip_enabled &&
        motor_rpm >= ctx->config_ptr->min_motor_speed_rpm && front_reliable;

    float rear_wheel_rpm = (float)motor_rpm / ctx->config_ptr->gear_ratio;

    float front_rpm_safe = front_reliable ? front_wheel_rpm : ctx->min_denominator_rpm;

    float slip_percent = (rear_wheel_rpm - front_rpm_safe) / front_rpm_safe * 100.0f;

    torque_request = compressor_apply(&ctx->slip, torque_request, slip_percent, slip_active);

    torque_request =
        compressor_apply(&ctx->power, torque_request, electrical_power_w, true);

    return torque_request;
}

void torque_limiters_reset(torque_limiters_context_t *ctx)
{
    compressor_reset(&ctx->slip);
    compressor_reset(&ctx->power);
}
