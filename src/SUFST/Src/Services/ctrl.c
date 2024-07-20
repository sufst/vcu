/*****************************************************************************
 * @file   driver_control.c
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @author Toby Godfrey (@_tg03, tmag1g21@soton.ac.uk)
 * @brief  Driver control
 ****************************************************************************/

#include "ctrl.h"

#include <stdbool.h>

#include "rtds.h"
#include "trc.h"

static log_context_t* log_h;

/*
 * internal function prototypes
 */
void ctrl_thread_entry(ULONG input);
void ctrl_state_machine_tick(ctrl_context_t* ctrl_ptr);
void ctrl_update_canbc_states(ctrl_context_t* ctrl_ptr);
void ctrl_handle_ts_fault(ctrl_context_t* ctrl_ptr);

/**
 * @brief       Initialises control service
 *
 * @param[in]   ctrl_ptr                Control context
 * @param[in]   dash_ptr                Dash context
 * @param[in]   canbc_ptr               CANBC context
 * @param[in]   pm100_ptr               PM100 context
 * @param[in]   stack_pool_ptr          Byte pool to allocate thread stack from
 * @param[in]   config_ptr              Configuration
 * @param[in]   apps_config_ptr         APPS configuration
 * @param[in]   bps_config_ptr          BPS configuration
 * @param[in]   rtds_config_ptr         RTDS configuration
 * @param[in]   torque_map_config_ptr   Torque map configuration
 */
status_t ctrl_init(ctrl_context_t* ctrl_ptr,
                   dash_context_t* dash_ptr,
                   pm100_context_t* pm100_ptr,
		   tick_context_t *tick_ptr,
                   canbc_context_t* canbc_ptr,
                   log_context_t* log_ptr,
                   TX_BYTE_POOL* stack_pool_ptr,
                   const config_ctrl_t* config_ptr,
                   const config_rtds_t* rtds_config_ptr,
                   const config_torque_map_t* torque_map_config_ptr)
{
     ctrl_ptr->state = CTRL_STATE_TS_BUTTON_WAIT;
     ctrl_ptr->dash_ptr = dash_ptr;
     ctrl_ptr->pm100_ptr = pm100_ptr;
     ctrl_ptr->tick_ptr = tick_ptr;
     ctrl_ptr->canbc_ptr = canbc_ptr;
     ctrl_ptr->config_ptr = config_ptr;
     ctrl_ptr->rtds_config_ptr = rtds_config_ptr;
     ctrl_ptr->error = CTRL_ERROR_NONE;
     ctrl_ptr->apps_reading = 0;
     ctrl_ptr->bps_reading = 0;
     ctrl_ptr->sagl_reading = 0;
     ctrl_ptr->torque_request = 0;
     ctrl_ptr->shdn_reading = 0;
     ctrl_ptr->precharge_start = 0;
     ctrl_ptr->inverter_pwr = false;
     ctrl_ptr->pump_pwr = false;
     ctrl_ptr->fan_pwr = false;
     
     log_h = log_ptr;

     // create the thread
     void* stack_ptr = NULL;
     UINT tx_status = tx_byte_allocate(stack_pool_ptr,
				       &stack_ptr,
				       config_ptr->thread.stack_size,
				       TX_NO_WAIT);

     if (tx_status == TX_SUCCESS)
     {
	  tx_status = tx_thread_create(&ctrl_ptr->thread,
				       (CHAR*) config_ptr->thread.name,
				       ctrl_thread_entry,
				       (ULONG) ctrl_ptr,
				       stack_ptr,
				       config_ptr->thread.stack_size,
				       config_ptr->thread.priority,
				       config_ptr->thread.priority,
				       TX_NO_TIME_SLICE,
				       TX_AUTO_START);
     }

     status_t status = (tx_status == TX_SUCCESS) ? STATUS_OK : STATUS_ERROR;

     // initialise the torque map
     if (status == STATUS_OK)
     {
	  status = torque_map_init(&ctrl_ptr->torque_map, torque_map_config_ptr);
     }

     // make sure TS is disabled
     trc_set_ts_on(GPIO_PIN_RESET);

     // check all ok before starting
     if (status != STATUS_OK)
     {
	  tx_thread_terminate(&ctrl_ptr->thread);
	  ctrl_ptr->error |= CTRL_ERROR_INIT;
     }

     // send initial state update
     ctrl_update_canbc_states(ctrl_ptr);

     return status;
}

/**
 * @brief       Control thread entry function
 *
 * @param[in]   input   Control context
 */
void ctrl_thread_entry(ULONG input)
{
     ctrl_context_t* ctrl_ptr = (ctrl_context_t*) input;

     while (1)
     {
	  uint32_t start_time = tx_time_get();
	  dash_update_buttons(ctrl_ptr->dash_ptr);

	  ctrl_ptr->shdn_reading = trc_ready();
	  //ctrl_ptr->motor_temp = pm100_motor_temp(ctrl_ptr->pm100_ptr);
	  //ctrl_ptr->inv_temp = pm100_max_inverter_temp(ctrl_ptr->pm100_ptr);
	  //LOG_INFO(log_h, "Motor temp: %d   Inverter temp: %d\n", ctrl_ptr->motor_temp,
	  //	   ctrl_ptr->inv_temp);
	  //ctrl_ptr->fan_pwr = ctrl_ptr->pump_pwr = (ctrl_ptr->motor_temp >= 100 ||
	  //					    ctrl_ptr->inv_temp >= 100);
	  ctrl_state_machine_tick(ctrl_ptr);
	  ctrl_update_canbc_states(ctrl_ptr);
	  uint32_t run_time = tx_time_get() - start_time;
	  
	  tx_thread_sleep(ctrl_ptr->config_ptr->schedule_ticks);
     }
}

/**
 * @brief       Runs one tick of the state machine for the control service
 *
 * @param[in]   ctrl_ptr    Control context
 */
void ctrl_state_machine_tick(ctrl_context_t* ctrl_ptr)
{
     // reduce typing...
     dash_context_t* dash_ptr = ctrl_ptr->dash_ptr;
     const config_ctrl_t* config_ptr = ctrl_ptr->config_ptr;

     ctrl_state_t next_state = ctrl_ptr->state;

     switch (ctrl_ptr->state)
     {

	  // wait for TS button to be held and released
	  // then begin activating the TS
     case (CTRL_STATE_TS_BUTTON_WAIT):
     {
	  if (dash_ptr->tson_flag)
	  {
	       dash_clear_buttons(dash_ptr);

	       if (trc_ready())
	       {
		    LOG_INFO(log_h, "TSON pressed & SHDN closed\n");

		    trc_set_ts_on(GPIO_PIN_SET);

		    next_state = CTRL_STATE_WAIT_NEG_AIR;
		    ctrl_ptr->neg_air_start = tx_time_get();
	       }
	  }

	  break;
     }

     case (CTRL_STATE_WAIT_NEG_AIR):
     {
	  if (tx_time_get() >= ctrl_ptr->neg_air_start + TX_TIMER_TICKS_PER_SECOND/4)
	  {
	       LOG_INFO(log_h, "Neg AIR closed, turning on inverter\n");

	       ctrl_ptr->inverter_pwr = true;

	       next_state = CTRL_STATE_PRECHARGE_WAIT;
	       ctrl_ptr->precharge_start = tx_time_get();
	  }

	  break;
     }

     // TS is ready, can initiate pre-charge sequence
     // TS on LED turns solid
     case (CTRL_STATE_PRECHARGE_WAIT):
     {
	  const uint32_t charge_time = tx_time_get() - ctrl_ptr->precharge_start;
	
	  if (pm100_is_precharged(ctrl_ptr->pm100_ptr))
	  {
	       next_state = CTRL_STATE_R2D_WAIT;
	       dash_clear_buttons(dash_ptr);
	       LOG_INFO(log_h, "Precharge complete\n");
	  }
	  else if (charge_time >= config_ptr->precharge_timeout_ticks)
	  {
	       ctrl_ptr->error |= CTRL_ERROR_PRECHARGE_TIMEOUT;
	       next_state = CTRL_STATE_TS_ACTIVATION_FAILURE;
	       LOG_ERROR(log_h, "Precharge timeout reached\n");
	  }

	  break;
     }

     // pre-charge is complete, wait for R2D signal
     // also wait for brake to be fully pressed (if enabled)
     case (CTRL_STATE_R2D_WAIT):
     {
	  if (!trc_ready())
	  {
	       LOG_ERROR(log_h, "SHDN opened\n");
	       next_state = CTRL_STATE_TS_ACTIVATION_FAILURE;
	  }
	  else if (dash_ptr->tson_flag) // TSON pressed, disable TS
	  {
	       dash_ptr->tson_flag = false;

	       ctrl_ptr->inverter_pwr = false; // Turn off inverter
	       trc_set_ts_on(GPIO_PIN_RESET); // Turn off AIRs

	       next_state = CTRL_STATE_TS_BUTTON_WAIT;
	  }
	  else if (dash_ptr->r2d_flag) // R2D pressed
	  {
	       dash_ptr->r2d_flag = false;

	       status_t result = tick_get_bps_reading(ctrl_ptr->tick_ptr,
						      &ctrl_ptr->bps_reading);
	  
	       bool r2d = false;

	       if (result == STATUS_OK)
	       {
		    r2d = (config_ptr->r2d_requires_brake) ? (ctrl_ptr->bps_reading > 3) : 1;
	       
		    if (r2d)
		    {
			 dash_set_r2d_led_state(dash_ptr, GPIO_PIN_SET);
			 pm100_disable(ctrl_ptr->pm100_ptr);
			 rtds_activate(ctrl_ptr->rtds_config_ptr, log_h);
			 
			 next_state = CTRL_STATE_TS_ON;
			 
			 LOG_INFO(log_h, "R2D active\n");
		    }
	       }
	       else
	       {
		    LOG_ERROR(log_h, "BPS reading failed\n");
		    next_state = CTRL_STATE_TS_ACTIVATION_FAILURE;
	       }
	  }
	  
	  break;
     }

     // the TS is on
     case (CTRL_STATE_TS_ON):
     {
	  // read from the APPS
	  status_t pm100_status;

	  status_t apps_status = tick_get_apps_reading(ctrl_ptr->tick_ptr,
						       &ctrl_ptr->apps_reading);
	  status_t bps_status = tick_get_bps_reading(ctrl_ptr->tick_ptr,
						     &ctrl_ptr->bps_reading);

	  if (dash_ptr->r2d_flag)
	  {
	       dash_clear_buttons(dash_ptr);
	       next_state = CTRL_STATE_R2D_OFF;
	  }
	  else if (apps_status == STATUS_OK && bps_status == STATUS_OK)
	  {
	       // Check for brake + accel pedal pressed
	       if (ctrl_ptr->apps_reading >=
		    ctrl_ptr->config_ptr->apps_bps_high_threshold &&
		    ctrl_ptr->bps_reading > 3)
	       {
		    LOG_ERROR(log_h, "BP and AP pressed\n");

		    if (tx_time_get() >= ctrl_ptr->apps_bps_start +
			(TX_TIMER_TICKS_PER_SECOND / 3))
		    {
			 LOG_ERROR(log_h, "BP-AP fault\n");
			 next_state = CTRL_STATE_APPS_BPS_FAULT;
		    }
	       }
	       else
	       {
		    ctrl_ptr->apps_bps_start = tx_time_get();
	       }
	       
	       ctrl_ptr->torque_request = torque_map_apply(&ctrl_ptr->torque_map,
							   ctrl_ptr->apps_reading);
	       
	       LOG_INFO(log_h, "ADC: %d, Torque: %d\n",
			ctrl_ptr->apps_reading, ctrl_ptr->torque_request);
	       
	       pm100_status
		    = pm100_request_torque(ctrl_ptr->pm100_ptr,
					   ctrl_ptr->torque_request);
	       
	       if (pm100_status != STATUS_OK)
	       {
		    next_state = CTRL_STATE_TS_RUN_FAULT;
	       }
	  }
	  else
	  {
	       LOG_ERROR(log_h, "APPS / BPS fault\n");
	       next_state = CTRL_STATE_TS_RUN_FAULT;
	  }

	  break;
     }

     case CTRL_STATE_R2D_OFF:
     {
	  ctrl_ptr->torque_request = 0;
	  status_t pm100_status = pm100_request_torque(ctrl_ptr->pm100_ptr, 0);
	  ctrl_ptr->motor_torque_zero_start = tx_time_get();
	  
	  if (pm100_status != STATUS_OK)
	  {
	       next_state = CTRL_STATE_TS_RUN_FAULT;
	  }
	  else
	  {
	       next_state = CTRL_STATE_R2D_OFF_WAIT;
	  }
	  break;
     }
     
     case CTRL_STATE_R2D_OFF_WAIT:
     {
	  ctrl_ptr->torque_request = 0;
	  status_t pm100_status = pm100_request_torque(ctrl_ptr->pm100_ptr, 0);
	  
	  if (pm100_status != STATUS_OK)
	  {
	       next_state = CTRL_STATE_TS_RUN_FAULT;
	  }
	  else if (tx_time_get() >= ctrl_ptr->motor_torque_zero_start +
		   TX_TIMER_TICKS_PER_SECOND/2)
	  {
	       next_state = CTRL_STATE_R2D_WAIT;
	       dash_set_r2d_led_state(dash_ptr, GPIO_PIN_RESET);
	  }
		   
	  break;
     }
     
     // activation or runtime failure
     case (CTRL_STATE_TS_ACTIVATION_FAILURE):
     case (CTRL_STATE_TS_RUN_FAULT):
     {
	  LOG_ERROR(log_h, "TS fault during activation or runtime\n");
	  ctrl_handle_ts_fault(ctrl_ptr);
	  next_state = CTRL_STATE_SPIN;
	  break;
     }

     case (CTRL_STATE_SPIN): // Spin forever
     {
	  break;
     }

     // SCS fault
     // this is recoverable, if the signal becomes plausible again
     case (CTRL_STATE_APPS_SCS_FAULT):
     {
	  ctrl_ptr->torque_request = 0;
	  status_t pm100_status = pm100_request_torque(ctrl_ptr->pm100_ptr, 0);

	  if (pm100_status != STATUS_OK)
	  {
	       next_state = CTRL_STATE_TS_RUN_FAULT;
	  }

	  if (tick_get_apps_reading(ctrl_ptr->tick_ptr, &ctrl_ptr->apps_reading) == STATUS_OK)
	  {
	       next_state = CTRL_STATE_TS_ON;
	  }

	  break;
     }

     case (CTRL_STATE_APPS_BPS_FAULT):
     {
	  ctrl_ptr->torque_request = 0;
	  status_t pm100_status = pm100_request_torque(ctrl_ptr->pm100_ptr, 0);

	  if (pm100_status != STATUS_OK)
	  {
	       next_state = CTRL_STATE_TS_RUN_FAULT;
	  }
	  
	  status_t apps_status = tick_get_apps_reading(ctrl_ptr->tick_ptr,
						       &ctrl_ptr->apps_reading);
	  status_t bps_status = tick_get_bps_reading(ctrl_ptr->tick_ptr,
						     &ctrl_ptr->bps_reading);
	  

	  if (apps_status == STATUS_OK && bps_status == STATUS_OK)
	  {	  
	       if ((ctrl_ptr->apps_reading < ctrl_ptr->config_ptr->apps_bps_low_threshold) &&
		   (ctrl_ptr->bps_reading == 0))
	       {
		    next_state = CTRL_STATE_TS_ON;
	       }
	  }
	  else
	  {
	       next_state = CTRL_STATE_APPS_SCS_FAULT;
	  }

	  break;
     }

     default:
	  break;
     }

     ctrl_ptr->state = next_state;
}

/**
 * @brief       Handles an initialisation or runtime fault of the TS and shuts
 *              down the service
 *
 * @param[in]   ctrl_ptr    Control context
 */
void ctrl_handle_ts_fault(ctrl_context_t* ctrl_ptr)
{
     dash_context_t* dash_ptr = ctrl_ptr->dash_ptr;
     const config_ctrl_t* config_ptr = ctrl_ptr->config_ptr;

     //pm100_lvs_off(ctrl_ptr->pm100_ptr);
     ctrl_ptr->inverter_pwr = false;
     ctrl_ptr->pump_pwr = false;
     ctrl_ptr->fan_pwr = false;
     
     trc_set_ts_on(GPIO_PIN_RESET);
     dash_blink_ts_on_led(dash_ptr, config_ptr->error_led_toggle_ticks);
     ctrl_update_canbc_states(ctrl_ptr);
}

/**
 * @brief       Updates the CAN broadcast states relevant to the control service
 *
 * @param[in]   ctrl_ptr
 */
void ctrl_update_canbc_states(ctrl_context_t* ctrl_ptr)
{
     canbc_states_t* states = canbc_lock_state(ctrl_ptr->canbc_ptr, TX_NO_WAIT);

     if (states != NULL)
     {
	  // TODO: add ready to drive state?
	  states->sensors.vcu_sagl = ctrl_ptr->sagl_reading;
	  states->sensors.vcu_torque_request = ctrl_ptr->torque_request;
	  states->state.vcu_ctrl_state = (uint8_t) ctrl_ptr->state;
	  states->state.vcu_drs_active = ctrl_ptr->shdn_reading;
	  states->errors.vcu_ctrl_error = ctrl_ptr->error;
	  states->pdm.inverter = ctrl_ptr->inverter_pwr;
	  //states->pdm.pump = ctrl_ptr->pump_pwr;
	  //states->pdm.fan = ctrl_ptr->fan_pwr;
	  canbc_unlock_state(ctrl_ptr->canbc_ptr);
     }
}
