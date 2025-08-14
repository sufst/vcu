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

/*
 * internal function prototypes
 */
void ctrl_thread_entry(ULONG input);
void ctrl_state_machine_tick(ctrl_context_t *ctrl_ptr);
void ctrl_update_canbc_states(ctrl_context_t *ctrl_ptr);
void ctrl_handle_ts_fault(ctrl_context_t *ctrl_ptr);
status_t ctrl_get_apps_reading(tick_context_t *tick_ptr, remote_ctrl_context_t *remote_ctrl_ptr, uint16_t *result);
status_t ctrl_get_bps_reading(tick_context_t *tick_ptr, remote_ctrl_context_t *remote_ctrl_ptr, uint16_t *result);
bool ctrl_fan_passed_on_threshold(ctrl_context_t* ctrl_ptr);
bool ctrl_fan_passed_off_threshold(ctrl_context_t* ctrl_ptr);

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
status_t ctrl_init(ctrl_context_t *ctrl_ptr,
				   dash_context_t *dash_ptr,
				   pm100_context_t *pm100_ptr,
				   tick_context_t *tick_ptr,
				   remote_ctrl_context_t *remote_ctrl_ptr,
				   canbc_context_t *canbc_ptr,
				   TX_BYTE_POOL *stack_pool_ptr,
				   const config_ctrl_t *config_ptr,
				   const config_rtds_t *rtds_config_ptr,
				   const config_torque_map_t *torque_map_config_ptr)
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
	ctrl_ptr->remote_ctrl_ptr = remote_ctrl_ptr;


	// create the thread
	void *stack_ptr = NULL;
	UINT tx_status = tx_byte_allocate(stack_pool_ptr,
									  &stack_ptr,
									  config_ptr->thread.stack_size,
									  TX_NO_WAIT);

	if (tx_status == TX_SUCCESS)
	{
		tx_status = tx_thread_create(&ctrl_ptr->thread,
									 (CHAR *)config_ptr->thread.name,
									 ctrl_thread_entry,
									 (ULONG)ctrl_ptr,
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
	ctrl_context_t *ctrl_ptr = (ctrl_context_t *)input;

	while (1)
	{
		uint32_t start_time = tx_time_get();
		dash_update_buttons(ctrl_ptr->dash_ptr);

        ctrl_ptr->shdn_reading = trc_ready();

        ctrl_ptr->motor_temp = pm100_motor_temp(ctrl_ptr->pm100_ptr);
        ctrl_ptr->inv_temp = pm100_max_inverter_temp(ctrl_ptr->pm100_ptr);
        ctrl_ptr->max_temp = ctrl_ptr->motor_temp > ctrl_ptr->inv_temp
                                 ? ctrl_ptr->motor_temp
                                 : ctrl_ptr->inv_temp;
        /*
        LOG_INFO("Motor temp: %d   Inverter temp: %d   Max temp: %d\n",

                 ctrl_ptr->motor_temp,
                 ctrl_ptr->inv_temp,
                 ctrl_ptr->max_temp);
        */
        if (ctrl_fan_passed_on_threshold(ctrl_ptr))
        {
            ctrl_ptr->fan_pwr = 1;
        }
        else if (ctrl_ptr->fan_pwr)
        {
            if (ctrl_fan_passed_off_threshold(ctrl_ptr))
            {
                ctrl_ptr->fan_pwr = 0;
            }
        }
        else
        {
            ctrl_ptr->fan_pwr = 0;
        }

        ctrl_state_machine_tick(ctrl_ptr);
        ctrl_update_canbc_states(ctrl_ptr);

        tx_thread_sleep(ctrl_ptr->config_ptr->schedule_ticks);
    }
}

/**
 * @brief       Checks the motor and inverter temperatures to determine if the
 * fan should be turned on
 *
 * @param[in]   ctrl_ptr    Control service pointer
 *
 * @return      True if the fan should be turned on
 */
bool ctrl_fan_passed_on_threshold(ctrl_context_t* ctrl_ptr)
{
    return ctrl_ptr->max_temp > ctrl_ptr->config_ptr->fan_on_threshold;
}

/**
 * @brief       Checks the motor and inverter temperatures to determine if the
 * fan should be turned off
 *
 * @param[in]   ctrl_ptr    Control service pointer
 *
 * @return      True if the fan should be turned off
 */
bool ctrl_fan_passed_off_threshold(ctrl_context_t* ctrl_ptr)
{
    return ctrl_ptr->max_temp < ctrl_ptr->config_ptr->fan_off_threshold;
}

/**
 * @brief       Runs one tick of the state machine for the control service
 *
 * @param[in]   ctrl_ptr    Control context
 */
void ctrl_state_machine_tick(ctrl_context_t *ctrl_ptr)
{
    // reduce typing...
    dash_context_t* dash_ptr = ctrl_ptr->dash_ptr;
	remote_ctrl_context_t *remote_ctrl_ptr = ctrl_ptr->remote_ctrl_ptr;
    const config_ctrl_t* config_ptr = ctrl_ptr->config_ptr;
    const uint16_t BPS_ON_THRESH = config_ptr->bps_on_threshold;

	ctrl_state_t next_state = ctrl_ptr->state;

	// In simulation mode, the TS and R2D buttons are controlled by the remote control, but the dash is still in effect
	#ifdef VCU_SIMULATION_MODE
		dash_ptr->tson_flag = dash_ptr->tson_flag || remote_get_ts_on_reading(remote_ctrl_ptr);
		dash_ptr->r2d_flag = dash_ptr->r2d_flag || remote_get_r2d_reading(remote_ctrl_ptr);
	#endif

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
				LOG_INFO("TSON pressed & SHDN closed\n");

				trc_set_ts_on(GPIO_PIN_SET);

				next_state = CTRL_STATE_WAIT_NEG_AIR;
				ctrl_ptr->neg_air_start = tx_time_get();
			}
		}
        else{
            ctrl_ptr->inverter_pwr = false; // Turn off inverter if TS button is not pressed
        }

		break;
	}

	case (CTRL_STATE_WAIT_NEG_AIR):
	{
		if (tx_time_get() >= ctrl_ptr->neg_air_start + TX_TIMER_TICKS_PER_SECOND / 4)
		{
			LOG_INFO("Neg AIR closed, turning on inverter\n");

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
			#ifdef VCU_SIMULATION_MODE
				next_state = CTRL_STATE_SIM_WAIT_TS_ON;
			#else
				next_state = CTRL_STATE_R2D_WAIT;
			#endif
			dash_clear_buttons(dash_ptr);
			LOG_INFO("Precharge complete\n");
		}
		else if (charge_time >= config_ptr->precharge_timeout_ticks)
		{
			ctrl_ptr->error |= CTRL_ERROR_PRECHARGE_TIMEOUT;
			next_state = CTRL_STATE_TS_ACTIVATION_FAILURE;
			LOG_ERROR("Precharge timeout reached\n");
		}

		break;
	}

	// pre-charge is complete, wait for R2D signal
	// also wait for brake to be fully pressed (if enabled)
	case (CTRL_STATE_R2D_WAIT):
	{
		if (!trc_ready())
		{
			LOG_ERROR("SHDN opened\n");
			next_state = CTRL_STATE_TS_ACTIVATION_FAILURE;
		}
		else if (dash_ptr->tson_flag) // TSON pressed, disable TS
		{
			dash_ptr->tson_flag = false;

			ctrl_ptr->inverter_pwr = false; // Turn off inverter
			trc_set_ts_on(GPIO_PIN_RESET);	// Turn off AIRs

			#ifdef VCU_SIMULATION_MODE
				next_state = CTRL_STATE_SIM_WAIT_TS_OFF;
			#else
				next_state = CTRL_STATE_TS_BUTTON_WAIT;
			#endif
		}
		else if (dash_ptr->r2d_flag) // R2D pressed
		{
			#ifndef VCU_SIMULATION_MODE
				dash_ptr->r2d_flag = false;
			#endif

			status_t result = ctrl_get_bps_reading(ctrl_ptr->tick_ptr,
												   remote_ctrl_ptr,
												   &ctrl_ptr->bps_reading);

			bool r2d = false;

			if (result == STATUS_OK)
			{
				r2d = (config_ptr->r2d_requires_brake) ? (ctrl_ptr->bps_reading > BPS_ON_THRESH) : 1;

				if (r2d)
				{
					dash_set_r2d_led_state(dash_ptr, GPIO_PIN_SET);
					pm100_disable(ctrl_ptr->pm100_ptr);
					rtds_activate(ctrl_ptr->rtds_config_ptr);
					ctrl_ptr->pump_pwr = 1;

					#ifdef VCU_SIMULATION_MODE
						next_state = CTRL_STATE_SIM_WAIT_R2D_ON;
					#else
						next_state = CTRL_STATE_TS_ON;
					#endif
					LOG_INFO("R2D active\n");
				}
			}
			else
			{
				LOG_ERROR("BPS reading failed\n");
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

		status_t apps_status = ctrl_get_apps_reading(ctrl_ptr->tick_ptr,
													 remote_ctrl_ptr,
													 &ctrl_ptr->apps_reading);
		status_t bps_status = ctrl_get_bps_reading(ctrl_ptr->tick_ptr,
												   remote_ctrl_ptr,
												   &ctrl_ptr->bps_reading);

		if (dash_ptr->r2d_flag)
		{
			dash_clear_buttons(dash_ptr);
			#ifdef VCU_SIMULATION_MODE
				next_state = CTRL_STATE_SIM_WAIT_R2D_OFF;
			#else
				next_state = CTRL_STATE_R2D_OFF;
			#endif
		}
		else if (apps_status == STATUS_OK && bps_status == STATUS_OK)
		{
			// Check for brake + accel pedal pressed
			if (ctrl_ptr->apps_reading >=
					ctrl_ptr->config_ptr->apps_bps_high_threshold &&
				ctrl_ptr->bps_reading > BPS_ON_THRESH)
			{
				LOG_ERROR("BP and AP pressed\n");

				if (tx_time_get() >= ctrl_ptr->apps_bps_start +
										 (TX_TIMER_TICKS_PER_SECOND / 3))
				{
					LOG_ERROR("BP-AP fault\n");
					// next_state = CTRL_STATE_APPS_BPS_FAULT;
				}
			}
			else
			{
				ctrl_ptr->apps_bps_start = tx_time_get();
			}
			#ifdef VCU_SIMULATION_MODE
				#ifndef VCU_SIMULATION_ON_POWER
					ctrl_ptr->torque_request = remote_get_torque_reading(remote_ctrl_ptr);
				#else
					uint16_t power = remote_get_power_reading(remote_ctrl_ptr);

					int16_t motor_speed = pm100_motor_speed(ctrl_ptr->pm100_ptr);
					uint16_t rad_s = 1;

					// this if to be removed
					if (motor_speed < 10)
					{
						motor_speed = 10;
					}
					// rpm to rad/s
					rad_s = (uint16_t)(motor_speed * 0.10472);
					if (rad_s == 0)
						rad_s = 1;
					ctrl_ptr->torque_request = (uint16_t)(power / rad_s);
					if (ctrl_ptr->torque_request > 1500)
						ctrl_ptr->torque_request = 1500;
				#endif
			#else
				ctrl_ptr->torque_request = torque_map_apply(&ctrl_ptr->torque_map,
															ctrl_ptr->apps_reading);
			#endif

			LOG_INFO("ADC: %d, Torque: %d\n",
					 ctrl_ptr->apps_reading, ctrl_ptr->torque_request);

			pm100_status = pm100_request_torque(ctrl_ptr->pm100_ptr,
												ctrl_ptr->torque_request);

			if (pm100_status != STATUS_OK)
			{
				next_state = CTRL_STATE_TS_RUN_FAULT;
			}
		}
		else
		{
			LOG_ERROR("APPS / BPS fault\n");
			next_state = CTRL_STATE_TS_RUN_FAULT;
		}

		break;
	}

	case CTRL_STATE_R2D_OFF:
	{
		ctrl_ptr->torque_request = 0;
		status_t pm100_status = pm100_request_torque(ctrl_ptr->pm100_ptr, 0);
		ctrl_ptr->motor_torque_zero_start = tx_time_get();
		ctrl_ptr->pump_pwr = 0;

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
        dash_set_r2d_led_state(dash_ptr, GPIO_PIN_RESET);
		ctrl_ptr->torque_request = 0;
		status_t pm100_status = pm100_request_torque(ctrl_ptr->pm100_ptr, 0);

		if (pm100_status != STATUS_OK)
		{
			next_state = CTRL_STATE_TS_RUN_FAULT;
		}
		else if (tx_time_get() >= ctrl_ptr->motor_torque_zero_start +
									  TX_TIMER_TICKS_PER_SECOND / 2)
		{
			#ifdef VCU_SIMULATION_MODE
				next_state = CTRL_STATE_SIM_WAIT_R2D_OFF;
			#else
				next_state = CTRL_STATE_R2D_WAIT;
			#endif

		}
		break;
	}

	// activation or runtime failure
	case (CTRL_STATE_TS_ACTIVATION_FAILURE):
	case (CTRL_STATE_TS_RUN_FAULT):
	{
		LOG_ERROR("TS fault during activation or runtime\n");
		ctrl_handle_ts_fault(ctrl_ptr);
		next_state = CTRL_STATE_TS_BUTTON_WAIT;
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

		if (ctrl_get_apps_reading(ctrl_ptr->tick_ptr, remote_ctrl_ptr, &ctrl_ptr->apps_reading) == STATUS_OK)
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

		status_t apps_status = ctrl_get_apps_reading(ctrl_ptr->tick_ptr,
													 remote_ctrl_ptr,
													 &ctrl_ptr->apps_reading);
		status_t bps_status = ctrl_get_bps_reading(ctrl_ptr->tick_ptr,
												   remote_ctrl_ptr,
												   &ctrl_ptr->bps_reading);

		if (apps_status == STATUS_OK && bps_status == STATUS_OK)
		{
			if ((ctrl_ptr->apps_reading < ctrl_ptr->config_ptr->apps_bps_low_threshold) &&
				(ctrl_ptr->bps_reading < BPS_ON_THRESH))
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

	// Needed when in simulation mode to avoid the TS being turned on again
	case (CTRL_STATE_SIM_WAIT_TS_OFF):
	{
		ctrl_ptr->torque_request = 0;
		status_t pm100_status = pm100_request_torque(ctrl_ptr->pm100_ptr, 0);
		if (pm100_status != STATUS_OK)
		{
			next_state = CTRL_STATE_TS_RUN_FAULT;
		}

		if (!dash_ptr->tson_flag)
		{
			next_state = CTRL_STATE_TS_BUTTON_WAIT;
		}
		break;
	}

	// Needed when in simulation mode to avoid the TS being turned off again
	case (CTRL_STATE_SIM_WAIT_TS_ON):
	{
		ctrl_ptr->torque_request = 0;
		status_t pm100_status = pm100_request_torque(ctrl_ptr->pm100_ptr, 0);
		if (pm100_status != STATUS_OK)
		{
			next_state = CTRL_STATE_TS_RUN_FAULT;
		}
		if (!dash_ptr->tson_flag)
		{
			next_state = CTRL_STATE_R2D_WAIT;
		}
		break;
	}

	// Needed when in simulation mode to avoid the R2D being turned off again
	case (CTRL_STATE_SIM_WAIT_R2D_ON):
	{
		ctrl_ptr->torque_request = 0;
		status_t pm100_status = pm100_request_torque(ctrl_ptr->pm100_ptr, 0);
		if (pm100_status != STATUS_OK)
		{
			next_state = CTRL_STATE_TS_RUN_FAULT;
		}
		if (!dash_ptr->r2d_flag)
		{
			next_state = CTRL_STATE_TS_ON;
		}
		break;
	}

	// Needed when in simulation mode to avoid the R2D being turned on again
	case (CTRL_STATE_SIM_WAIT_R2D_OFF):
	{
		ctrl_ptr->torque_request = 0;
		status_t pm100_status = pm100_request_torque(ctrl_ptr->pm100_ptr, 0);
		if (pm100_status != STATUS_OK)
		{
			next_state = CTRL_STATE_TS_RUN_FAULT;
		}
		if (!dash_ptr->r2d_flag)
		{
			dash_set_r2d_led_state(dash_ptr, GPIO_PIN_RESET);
			next_state = CTRL_STATE_R2D_WAIT;
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
void ctrl_handle_ts_fault(ctrl_context_t *ctrl_ptr)
{
	dash_context_t *dash_ptr = ctrl_ptr->dash_ptr;
	const config_ctrl_t *config_ptr = ctrl_ptr->config_ptr;

	// pm100_lvs_off(ctrl_ptr->pm100_ptr);
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
void ctrl_update_canbc_states(ctrl_context_t *ctrl_ptr)
{
	canbc_states_t *states = canbc_lock_state(ctrl_ptr->canbc_ptr, TX_NO_WAIT);

	if (states != NULL)
	{
		// TODO: add ready to drive state?
		states->sensors.vcu_sagl = ctrl_ptr->sagl_reading;
		states->sensors.vcu_torque_request = ctrl_ptr->torque_request;
		states->temps.vcu_max_temp = (int8_t) ctrl_ptr->max_temp;
		states->state.vcu_ctrl_state = (uint8_t)ctrl_ptr->state;
		states->state.vcu_drs_active = ctrl_ptr->shdn_reading;
		states->errors.vcu_ctrl_error = ctrl_ptr->error;
		states->pdm.inverter = ctrl_ptr->inverter_pwr;
		states->pdm.pump = ctrl_ptr->pump_pwr;
		states->pdm.fan = ctrl_ptr->fan_pwr;
		canbc_unlock_state(ctrl_ptr->canbc_ptr);
	}
}

status_t ctrl_get_apps_reading(tick_context_t *tick_ptr, remote_ctrl_context_t *remote_ctrl_ptr, uint16_t *result)
{
	#ifdef VCU_SIMULATION_MODE
		return remote_get_apps_reading(remote_ctrl_ptr, result);
	#else
		return tick_get_apps_reading(tick_ptr, result);
	#endif
}
status_t ctrl_get_bps_reading(tick_context_t *tick_ptr, remote_ctrl_context_t *remote_ctrl_ptr, uint16_t *result)
{
	#ifdef VCU_SIMULATION_MODE
		return remote_get_bps_reading(remote_ctrl_ptr, result);
	#else
		return tick_get_bps_reading(tick_ptr, result);
	#endif
}