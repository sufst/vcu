#include "tick.h"

static status_t lock_tick_sensors(tick_context_t *tick_ptr, uint32_t timeout);
static void unlock_tick_sensors(tick_context_t *tick_ptr);
void tick_update_canbc_states(tick_context_t* tick_ptr);

static void tick_thread_entry(ULONG input)
{
     tick_context_t *tick_ptr = (tick_context_t *) input;
     const config_tick_t *config_ptr = tick_ptr->config_ptr;

     while (1)
     {
	  lock_tick_sensors(tick_ptr, 100);
	  tick_ptr->bps_status = bps_read(&tick_ptr->bps, &tick_ptr->bps_reading);
	  tick_ptr->brakelight_pwr = (tick_ptr->bps_reading > 284);

	  tick_ptr->apps_status = apps_read(&tick_ptr->apps, &tick_ptr->apps_reading);
	  
	  //LOG_INFO(tick_ptr->log_ptr, "Brake pressed: %d\n", tick_ptr->brakelight_pwr);
	  //LOG_INFO(tick_ptr->log_ptr, "APPS: %d   status: %d\n",
	  //	   tick_ptr->apps_reading, tick_ptr->apps_status);

	  tick_update_canbc_states(tick_ptr);
	  unlock_tick_sensors(tick_ptr);

	  tx_thread_sleep(config_ptr->period);
     }
}

status_t tick_init(tick_context_t *tick_ptr,
		   log_context_t *log_ptr,
		   canbc_context_t *canbc_ptr,
		   TX_BYTE_POOL *stack_pool_ptr,
		   const config_tick_t *config_ptr,
		   const config_apps_t* apps_config_ptr,
                   const config_bps_t* bps_config_ptr)
{
     tick_ptr->config_ptr = config_ptr;
     tick_ptr->canbc_ptr = canbc_ptr;
     tick_ptr->log_ptr = log_ptr;

     // Assume error so that it won't proceed without at least 1 reading
     tick_ptr->bps_status = STATUS_ERROR;
     tick_ptr->apps_status = STATUS_ERROR;

     status_t status = STATUS_OK;

     void *stack_ptr = NULL;
     UINT tx_status = tx_byte_allocate(stack_pool_ptr,
				       &stack_ptr,
				       config_ptr->thread.stack_size,
				       TX_NO_WAIT);

     // create state mutex
     if (tx_status == TX_SUCCESS)
     {
	  tx_status = tx_mutex_create(&tick_ptr->sensor_mutex, NULL, TX_INHERIT);
     }
     // initialise the APPS and BPS
     if (status == STATUS_OK)
     {
	  status = apps_init(&tick_ptr->apps, apps_config_ptr);
     }

     if (status == STATUS_OK)
     {
	  status = bps_init(&tick_ptr->bps, bps_config_ptr);
     }
     
     if (tx_status == TX_SUCCESS)
     {
	  tx_status = tx_thread_create(&tick_ptr->thread,
				       (CHAR*) config_ptr->thread.name,
				       tick_thread_entry,
				       (ULONG) tick_ptr,
				       stack_ptr,
				       config_ptr->thread.stack_size,
				       config_ptr->thread.priority,
				       config_ptr->thread.priority,
				       TX_NO_TIME_SLICE,
				       TX_AUTO_START);
     }
     if (tx_status != TX_SUCCESS)
     {
	  status = STATUS_ERROR;
     }
     
     // check all ok
     if (status != STATUS_OK)
     {
	  tx_thread_terminate(&tick_ptr->thread);
     }

     return status;
}

void tick_update_canbc_states(tick_context_t* tick_ptr)
{
     canbc_states_t* states = canbc_lock_state(tick_ptr->canbc_ptr, TX_NO_WAIT);

     if (states != NULL)
     {
	  states->pdm.brakelight = tick_ptr->brakelight_pwr;
	  canbc_unlock_state(tick_ptr->canbc_ptr);
     }
}

static status_t lock_tick_sensors(tick_context_t *tick_ptr, uint32_t timeout)
{
    UINT tx_status = tx_mutex_get(&tick_ptr->sensor_mutex, timeout);
    
    if (tx_status == TX_SUCCESS)
    {
	 return STATUS_OK;
    }
    else
    {
	 return STATUS_ERROR;
    }
}

static void unlock_tick_sensors(tick_context_t *tick_ptr)
{
     tx_mutex_put(&tick_ptr->sensor_mutex);
}

status_t tick_get_bps_reading(tick_context_t *tick_ptr, uint16_t *result)
{
     status_t status = STATUS_ERROR;
     
     if (lock_tick_sensors(tick_ptr, 100) == STATUS_OK)
     {
	  *result = tick_ptr->bps_reading;
	  status = tick_ptr->bps_status;
	  unlock_tick_sensors(tick_ptr);
     }
     else
     {
	  LOG_ERROR(tick_ptr->log_ptr, "BPS locking error\n");
     }

     return status;
}

status_t tick_get_apps_reading(tick_context_t *tick_ptr, uint16_t *result)
{
     status_t status = STATUS_ERROR;
     
     if (lock_tick_sensors(tick_ptr, 100) == STATUS_OK)
     {
	  *result = tick_ptr->apps_reading;
	  status = tick_ptr->apps_status;
	  unlock_tick_sensors(tick_ptr);
     }
     else
     {
	  LOG_ERROR(tick_ptr->log_ptr, "APPS locking error\n");
     }

     return status;
}
