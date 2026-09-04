#include "compressor.h"

void compressor_init(compressor_context_t *ctx, const config_compressor_t *config_ptr)
{
    ctx->config_ptr = config_ptr;
    ctx->integral = 0.0f;
}

uint16_t compressor_apply(compressor_context_t *ctx, uint16_t torque_request, float measured_value, bool gate_active)
{
    const config_compressor_t *cfg = ctx->config_ptr;
    float excess = gate_active ? (measured_value - cfg->threshold) : 0.0f;

    float derate = (excess > 0.0f) ? excess * cfg->p_gain : 0.0f;

    if (cfg->integral_enabled)
    {
        ctx->integral *= cfg->integral_decay;

        if (excess > 0.0f)
        {
            ctx->integral += excess * cfg->i_gain;
        }

        if (ctx->integral > cfg->integral_max)
        {
            ctx->integral = cfg->integral_max;
        }
        else if (ctx->integral < 0.0f)
        {
            ctx->integral = 0.0f;
        }

        derate += ctx->integral;
    }

    if (!gate_active || derate <= 0.0f)
    {
        return torque_request;
    }

    float limited = (float)torque_request - derate;
    return (limited > 0.0f) ? (uint16_t)limited : 0;
}

void compressor_reset(compressor_context_t *ctx)
{
    ctx->integral = 0.0f;
}
