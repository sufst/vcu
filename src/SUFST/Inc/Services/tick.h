#ifndef TICK_H
#define TICK_H

#include <tx_api.h>

#include "config.h"
#include "log.h"
#include "status.h"
#include "canbc.h"
#include "bps.h"
#include "apps.h"
#include "sagl.h"

typedef struct
{
     TX_THREAD thread;
     TX_MUTEX sensor_mutex;
     const config_tick_t *config_ptr;
     canbc_context_t *canbc_ptr;
     log_context_t *log_ptr;

     bps_context_t bps;
     bool brakelight_pwr;
     bool pump_pwr, fan_pwr;
     apps_context_t apps;
     sagl_context_t sagl;
     status_t bps_status, apps_status, sagl_status;
     uint16_t bps_reading, apps_reading;
     int16_t sagl_reading;
} tick_context_t;

status_t tick_init(tick_context_t *tick_ptr,
		   log_context_t *log_ptr,
		   canbc_context_t *canbc_ptr,
		   TX_BYTE_POOL *stack_pool_ptr,
		   const config_tick_t *config_ptr,
		   const config_apps_t* apps_config_ptr,
                   const config_bps_t* bps_config_ptr,
                   const config_sagl_t * sagl_config_ptr);

status_t tick_get_bps_reading(tick_context_t *tick_ptr, uint16_t *result);
status_t tick_get_apps_reading(tick_context_t *tick_ptr, uint16_t *result);
status_t tick_get_sagl_reading(tick_context_t *tick_ptr, int16_t *result);

#endif /* TICK_H */
