#ifndef TICK_H
#define TICK_H

#include <tx_api.h>

#include "apps.h"
#include "bps.h"
#include "canbc.h"
#include "config.h"
#include "log.h"
#include "scs.h"
#include "status.h"

typedef struct
{
    TX_THREAD thread;
    TX_MUTEX sensor_mutex;
    const config_tick_t *config_ptr;
    const config_ext_inputs_t *ext_inputs_config_ptr;
    canbc_context_t *canbc_ptr;

    bps_context_t bps;
    bool brakelight_pwr;
    bool bps_prev_above;
    uint32_t bps_active_start;
    bool pump_pwr, fan_pwr;
    apps_context_t apps;
    status_t bps_status, apps_status, sagl_status, mode_adc_status, current_status;
    uint16_t bps_reading, apps_reading, sagl_reading, mode_adc_reading, current_reading;

    scs_t sagl;
    scs_t current;
    scs_t mode_adc;
} tick_context_t;

status_t tick_init(tick_context_t *tick_ptr,
                   canbc_context_t *canbc_ptr,
                   TX_BYTE_POOL *stack_pool_ptr,
                   const config_tick_t *config_ptr,
                   const config_apps_t *apps_config_ptr,
                   const config_bps_t *bps_config_ptr,
                   const config_ext_inputs_t *ext_inputs_config_ptr);

status_t tick_get_bps_reading(tick_context_t *tick_ptr, uint16_t *result);
status_t tick_get_apps_reading(tick_context_t *tick_ptr, uint16_t *result);
status_t tick_get_sagl_reading(tick_context_t *tick_ptr, uint16_t *result);
status_t tick_get_current_reading(tick_context_t *tick_ptr, uint16_t *result);
status_t tick_get_mode_adc_reading(tick_context_t *tick_ptr, uint16_t *result);
status_t tick_clear_apps_scs_error(tick_context_t *tick_ptr);

#endif /* TICK_H */
