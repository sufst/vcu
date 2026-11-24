#include "pm100.h"

#include <math.h>

#include <can_s.h>
#include <can_t.h>

#include "io.h"
#include "rtos_utils.h"

#define PM100_NO_FAULTS 0x00

#define PM100_VSM_STATE_START 0x00
#define PM100_VSM_STATE_PRECHARGE_INIT 0x01
#define PM100_VSM_STATE_PRECHARGE_ACTIVE 0x02
#define PM100_VSM_STATE_PRECHARGE_COMPLETE 0x03
#define PM100_VSM_STATE_WAIT 0x04
#define PM100_VSM_STATE_READY 0x05
#define PM100_VSM_STATE_RUNNING 0x06
#define PM100_VSM_STATE_FAULT 0x07

#define PM100_LOCKOUT_DISABLED 0x0
#define PM100_LOCKOUT_ENABLED 0x1
#define PM100_CAN_MODE 0x0
#define PM100_VSM_MODE 0x1
#define PM100_DIRECTION_REVERSE 0x0
#define PM100_DIRECTION_FORWARD 0x1
#define PM100_INVERTER_OFF 0x0
#define PM100_INVERTER_ON 0x1
#define PM100_TORQUE_MODE 0x0
#define PM100_SPEED_MODE 0x1
#define PM100_SPEED_MODE_DISABLE 0x0
#define PM100_SPEED_MODE_ENABLE 0x1

#define PM100_DIRECTION_FORWARD 0x1
#define PM100_DIRECTION_REVERSE 0x0

/*
 * internal function prototypes
 */
static void pm100_thread_entry(void *arg);
static bool process_broadcast(pm100_context_t *pm100_ptr, const rtcan_msg_t *msg_ptr);

/**
 * @brief   Initialises the PM100 service
 *
 * @param[in]   pm100_ptr       PM100 context
 * @param[in]   rtcan_t_ptr     RTCAN T instance for receiving broadcasts
 * @param[in]   rtcan_s_ptr     RTCAN S instance for sending precharge cmd
 * @param[in]   config_ptr      Configuration
 */
status_t pm100_init(pm100_context_t *pm100_ptr,
                    rtcan_handle_t *rtcan_t_ptr,
                    rtcan_handle_t *rtcan_s_ptr,
                    const config_pm100_t *config_ptr)
{
    pm100_ptr->config_ptr = config_ptr;
    pm100_ptr->rtcan_t_ptr = rtcan_t_ptr;
    pm100_ptr->rtcan_s_ptr = rtcan_s_ptr;
    pm100_ptr->error = PM100_ERROR_NONE;
    pm100_ptr->request_failed = false;
    pm100_ptr->lockout_logged = false;
    pm100_ptr->broadcasts_valid = false;

    status_t status = STATUS_OK;

    // create CAN receive queue
    if (rtcan_os_queue_create(&pm100_ptr->can_rx_queue, NULL, sizeof(rtcan_msg_t *),
                              PM100_RX_QUEUE_SIZE, pm100_ptr->can_rx_queue_mem,
                              sizeof(pm100_ptr->can_rx_queue_mem)) != RTCAN_OS_OK)
    {
        status = STATUS_ERROR;
    }

    // create state mutex
    if (status == STATUS_OK)
    {
        pm100_ptr->state_mutex = xSemaphoreCreateMutexStatic(&pm100_ptr->state_mutex_cb);

        if (pm100_ptr->state_mutex == NULL)
        {
            status = STATUS_ERROR;
        }
    }

    // create service thread (only once everything it uses exists)
    if (status == STATUS_OK)
    {
        status = rtos_task_create(&config_ptr->thread, pm100_thread_entry, pm100_ptr,
                                  &pm100_ptr->thread);
    }

    // turn off power
    VCU_Output_Low(STATUS_GPIO_Port, // This pin used to be called STATUS
                   STATUS_Pin);

    return status;
}

/**
 * @brief   PM100 service thread entry function
 *
 * @details This thread is responsible for receiving and processing broadcast
 *          messages from the PM100
 */
static void pm100_thread_entry(void *arg)
{
    pm100_context_t *pm100_ptr = (pm100_context_t *)arg;
    const config_pm100_t *config_ptr = pm100_ptr->config_ptr;

    // set up RTCAN subscriptions
    uint32_t subscriptions[] = { CAN_T_PM100_INTERNAL_STATES_FRAME_ID,
                                 CAN_T_PM100_FAULT_CODES_FRAME_ID,
                                 CAN_T_PM100_TEMPERATURE_SET_1_FRAME_ID,
                                 CAN_T_PM100_TEMPERATURE_SET_2_FRAME_ID,
                                 CAN_T_PM100_TEMPERATURE_SET_3_FRAME_ID,
                                 CAN_T_PM100_MOTOR_POSITION_INFO_FRAME_ID,
                                 CAN_T_PM100_VOLTAGE_INFO_FRAME_ID,
                                 CAN_T_PM100_CURRENT_INFO_FRAME_ID };

    for (uint32_t i = 0; i < sizeof(subscriptions) / sizeof(subscriptions[0]); i++)
    {
        rtcan_status_t status =
            rtcan_subscribe(pm100_ptr->rtcan_t_ptr, subscriptions[i], pm100_ptr->can_rx_queue);

        if (status != RTCAN_OK)
        {
            LOG_ERROR("Could not subscribe on %d message. Terminating thread\n", i);
            vTaskDelete(NULL);
        }
    }

    // process incoming messages, or timeout
    while (1)
    {
        rtcan_msg_t *msg_ptr = NULL;
        rtcan_osal_status_t status =
            rtcan_os_queue_receive(pm100_ptr->can_rx_queue, &msg_ptr,
                                   config_ptr->broadcast_timeout_ticks);

        if (status == RTCAN_OS_TIMEOUT)
        {
            // timed out
            // TODO: error
            if (xSemaphoreTake(pm100_ptr->state_mutex, 100) == pdPASS)
            {
                pm100_ptr->broadcasts_valid = false;
                (void)xSemaphoreGive(pm100_ptr->state_mutex);
            }
            LOG_INFO("PM100 broadcast timeout\n");
        }
        else if (status == RTCAN_OS_OK && msg_ptr != NULL)
        {
            // the control task reads this state, so only update it while
            // holding the state mutex
            bool disable = false;
            if (xSemaphoreTake(pm100_ptr->state_mutex, 100) == pdPASS)
            {
                pm100_ptr->broadcasts_valid = true;
                disable = process_broadcast(pm100_ptr, msg_ptr);
                (void)xSemaphoreGive(pm100_ptr->state_mutex);
            }
            else
            {
                LOG_ERROR("PM100 thread: state locking error\n");
            }
            rtcan_msg_consumed(pm100_ptr->rtcan_t_ptr, msg_ptr);

            // send the disable command outside the lock
            if (disable)
            {
                (void)pm100_disable(pm100_ptr);
            }
        }
        else
        {
            LOG_ERROR("PM100 thread: unexpected error %d\n", status);
        }
    }
}

/**
 * @brief   Processes incoming broadcast messages
 *
 * @details     Must be called with the state mutex held
 *
 * @param[in]   pm100_ptr   PM100 context
 * @param[in]   msg_ptr     Incoming message
 *
 * @return      True if the inverter reported a fault and must be disabled
 */
static bool process_broadcast(pm100_context_t *pm100_ptr, const rtcan_msg_t *msg_ptr)
{
    bool disable = false;

    switch (msg_ptr->identifier)
    {
    case CAN_T_PM100_INTERNAL_STATES_FRAME_ID:
    {
        can_t_pm100_internal_states_unpack(&pm100_ptr->states, msg_ptr->data,
                                           msg_ptr->length);

        break;
    }

    case CAN_T_PM100_FAULT_CODES_FRAME_ID:
    {
        can_t_pm100_fault_codes_unpack(&pm100_ptr->faults, msg_ptr->data,
                                       msg_ptr->length);

        if (pm100_ptr->faults.pm100_run_fault_lo != PM100_NO_FAULTS ||
            pm100_ptr->faults.pm100_run_fault_hi != PM100_NO_FAULTS)
        {
            pm100_ptr->error |= PM100_ERROR_RUN_FAULT;
            disable = true;
        }
        else if (pm100_ptr->faults.pm100_post_fault_lo != PM100_NO_FAULTS ||
                 pm100_ptr->faults.pm100_post_fault_hi != PM100_NO_FAULTS)
        {
            pm100_ptr->error |= PM100_ERROR_POST_FAULT;
            disable = true;
        }

        break;
    }

    case CAN_T_PM100_TEMPERATURE_SET_1_FRAME_ID:
    {
        can_t_pm100_temperature_set_1_unpack(&pm100_ptr->temp1, msg_ptr->data,
                                             msg_ptr->length);

        break;
    }

    case CAN_T_PM100_TEMPERATURE_SET_2_FRAME_ID:
    {
        can_t_pm100_temperature_set_2_unpack(&pm100_ptr->temp2, msg_ptr->data,
                                             msg_ptr->length);

        break;
    }

    case CAN_T_PM100_TEMPERATURE_SET_3_FRAME_ID:
    {
        can_t_pm100_temperature_set_3_unpack(&pm100_ptr->temp3, msg_ptr->data,
                                             msg_ptr->length);

        break;
    }

    case CAN_T_PM100_MOTOR_POSITION_INFO_FRAME_ID:
    {
        can_t_pm100_motor_position_info_unpack(&pm100_ptr->info, msg_ptr->data,
                                               msg_ptr->length);
        break;
    }

    case CAN_T_PM100_VOLTAGE_INFO_FRAME_ID:
    {
        can_t_pm100_voltage_info_unpack(&pm100_ptr->voltage_info, msg_ptr->data,
                                        msg_ptr->length);
        break;
    }

    case CAN_T_PM100_CURRENT_INFO_FRAME_ID:
    {
        can_t_pm100_current_info_unpack(&pm100_ptr->current_info, msg_ptr->data,
                                        msg_ptr->length);
        break;
    }

    default: break;
    }

    return disable;
}

/**
 * @brief       Checks if the PM100 is precharged
 *
 * @param[in]   pm100_ptr   PM100 context
 */
bool pm100_is_precharged(pm100_context_t *pm100_ptr)
{
    BaseType_t mutex_status = xSemaphoreTake(pm100_ptr->state_mutex, 100);

    bool broadcasts_valid = false;
    uint8_t vsm_state = PM100_VSM_STATE_FAULT; // assume something safe

    if (mutex_status == pdPASS)
    {
        vsm_state = pm100_ptr->states.pm100_vsm_state;
        broadcasts_valid = pm100_ptr->broadcasts_valid;
        (void)xSemaphoreGive(pm100_ptr->state_mutex);
    }

    return broadcasts_valid &&
        (vsm_state == PM100_VSM_STATE_PRECHARGE_COMPLETE || vsm_state == PM100_VSM_STATE_WAIT ||
         vsm_state == PM100_VSM_STATE_READY || vsm_state == PM100_VSM_STATE_RUNNING);
}

int16_t pm100_max_inverter_temp(pm100_context_t *pm100_ptr)
{
    int16_t max_temp = 0;

    BaseType_t mutex_status = xSemaphoreTake(pm100_ptr->state_mutex, 100);

    if (mutex_status == pdPASS)
    {
        int16_t module_a = (int16_t)can_t_pm100_temperature_set_1_pm100_module_a_decode(
            pm100_ptr->temp1.pm100_module_a);
        int16_t module_b = (int16_t)can_t_pm100_temperature_set_1_pm100_module_b_decode(
            pm100_ptr->temp1.pm100_module_b);
        int16_t module_c = (int16_t)can_t_pm100_temperature_set_1_pm100_module_c_decode(
            pm100_ptr->temp1.pm100_module_c);
        int16_t gate_driver_board =
            (int16_t)can_t_pm100_temperature_set_1_pm100_gate_driver_board_decode(
                pm100_ptr->temp1.pm100_gate_driver_board);
        int16_t control_board =
            (int16_t)can_t_pm100_temperature_set_2_pm100_control_board_temperature_decode(
                pm100_ptr->temp2.pm100_control_board_temperature);

        if (module_a > max_temp)
            max_temp = module_a;
        if (module_b > max_temp)
            max_temp = module_b;
        if (module_c > max_temp)
            max_temp = module_c;
        if (gate_driver_board > max_temp)
            max_temp = gate_driver_board;
        if (control_board > max_temp)
            max_temp = control_board;

        xSemaphoreGive(pm100_ptr->state_mutex);
    }
    else
    {
        LOG_ERROR("Failed to get max temp\n");
    }

    return max_temp;
}

int16_t pm100_motor_temp(pm100_context_t *pm100_ptr)
{
    int16_t motor_temp = 0;

    BaseType_t mutex_status = xSemaphoreTake(pm100_ptr->state_mutex, 100);

    if (mutex_status == pdPASS)
    {
        motor_temp = (int16_t)can_t_pm100_temperature_set_3_pm100_motor_temperature_decode(
            pm100_ptr->temp3.pm100_motor_temperature);

        xSemaphoreGive(pm100_ptr->state_mutex);
    }
    else
    {
        LOG_ERROR("Failed to get motor temp\n");
    }

    return motor_temp;
}

int16_t pm100_motor_speed(pm100_context_t *pm100_ptr)
{
    int16_t speed = 0;

    BaseType_t mutex_status = xSemaphoreTake(pm100_ptr->state_mutex, 100);

    if (mutex_status == pdPASS)
    {
        speed = pm100_ptr->info.pm100_motor_speed;
        xSemaphoreGive(pm100_ptr->state_mutex);
    }

    return speed;
}

float pm100_electrical_power_w(pm100_context_t *pm100_ptr)
{
    // return infinity on failure
    float power_w = INFINITY;

    BaseType_t mutex_status = xSemaphoreTake(pm100_ptr->state_mutex, 100);

    if (mutex_status == pdPASS)
    {
        float voltage = (float)can_t_pm100_voltage_info_pm100_dc_bus_voltage_decode(
            pm100_ptr->voltage_info.pm100_dc_bus_voltage);
        float current = (float)can_t_pm100_current_info_pm100_dc_bus_current_decode(
            pm100_ptr->current_info.pm100_dc_bus_current);

        power_w = voltage * current;
        xSemaphoreGive(pm100_ptr->state_mutex);
    }
    else
    {
        LOG_ERROR("Failed to get electrical power\n");
    }

    return power_w;
}

/**
 * @brief       Forgets the last inverter state received
 *
 * @details     Called when a new precharge starts, so that a state received
 *              before the inverter was last switched off (e.g. 'ready') can't
 *              be taken as 'precharge complete': only a state the inverter
 *              broadcasts from now on counts.
 *
 * @param[in]   pm100_ptr   PM100 context
 */
void pm100_forget_state(pm100_context_t *pm100_ptr)
{
    if (xSemaphoreTake(pm100_ptr->state_mutex, 100) == pdPASS)
    {
        pm100_ptr->states.pm100_vsm_state = PM100_VSM_STATE_START;
        pm100_ptr->broadcasts_valid = false;
        (void)xSemaphoreGive(pm100_ptr->state_mutex);
    }
}

/**
 * @brief       Clears the latched PM100 error flags
 * @param[in]   pm100_ptr   PM100 context
 */
void pm100_clear_error(pm100_context_t *pm100_ptr)
{
    BaseType_t mutex_status = xSemaphoreTake(pm100_ptr->state_mutex, 100);

    if (mutex_status == pdPASS)
    {
        pm100_ptr->error = PM100_ERROR_NONE;
        xSemaphoreGive(pm100_ptr->state_mutex);
    }
}

status_t pm100_lvs_off(pm100_context_t *pm100_ptr)
{
    return STATUS_OK;
}

/**
 * @brief       Disables the inverter
 *
 * @details     A disable command has data all zero. A disable command must be
 *              sent before enabling the inverter to exit the lockout state.
 *
 * @param[in]   pm100_ptr
 */
status_t pm100_disable(pm100_context_t *pm100_ptr)
{
    LOG_DEBUG("Sending PM100 Disable command\n");
    rtcan_msg_t msg = { .identifier = CAN_T_PM100_COMMAND_MESSAGE_FRAME_ID,
                        .length = CAN_T_PM100_COMMAND_MESSAGE_LENGTH,
                        .extended = CAN_T_PM100_COMMAND_MESSAGE_IS_EXTENDED,
                        .data = { 0, 0, 0, 0, 0, 0, 0, 0 } };

    rtcan_status_t status = rtcan_transmit(pm100_ptr->rtcan_t_ptr, &msg);

    return (status == RTCAN_OK) ? STATUS_OK : STATUS_ERROR;
}

/**
 * @brief       Sends a torque request to the PM100
 *
 * @details     If the PM100 is in a fault state, or the lockout state is
 *              unknown the torque request will not be sent.
 *
 *              If the inverter is in lockout, a 'disable' command will be sent
 *              instead of the torque request to get the inverter out of
 *              lockout. If lockout is successfully exited, the next torque
 *              request will actually be sent.
 *
 * @param[in]   pm100_ptr   PM100 context
 * @param[in]   torque      Desired torque
 */
status_t pm100_request_torque(pm100_context_t *pm100_ptr, uint16_t torque)
{

    status_t status = STATUS_OK;
    const bool is_precharged =
        pm100_is_precharged(pm100_ptr); // do this before getting mutex to avoid deadlock
    const bool no_errors = (pm100_ptr->error == PM100_ERROR_NONE);

    BaseType_t mutex_status = xSemaphoreTake(pm100_ptr->state_mutex,
                                             pm100_ptr->config_ptr->torque_request_timeout_ticks);

    if (no_errors && is_precharged && mutex_status == pdPASS)
    {
        pm100_ptr->request_failed = false;

        if (pm100_ptr->broadcasts_valid)
        {
            if (pm100_ptr->states.pm100_inverter_enable_lockout == PM100_LOCKOUT_DISABLED)
            {
                pm100_ptr->lockout_logged = false;
                bool inverter_enable = PM100_INVERTER_ON;

                rtcan_msg_t msg = { .identifier = CAN_T_PM100_COMMAND_MESSAGE_FRAME_ID,
                                    .length = CAN_T_PM100_COMMAND_MESSAGE_LENGTH,
                                    .extended = CAN_T_PM100_COMMAND_MESSAGE_IS_EXTENDED,
                                    .data = { 0, 0, 0, 0, 0, 0, 0, 0 } };

                // state mutex is already held here, so read the speed
                // directly rather than through pm100_motor_speed() (FreeRTOS
                // mutexes are not recursive, unlike ThreadX mutexes)
                // the motor speed is signed (the motor is wired backwards)
                int16_t speed = pm100_ptr->info.pm100_motor_speed;
                if (torque == 0 && speed > -10 && speed < 10)
                {
                    inverter_enable = PM100_INVERTER_OFF;
                }

                struct can_t_pm100_command_message_t cmd = {
                    .pm100_torque_command = torque,
                    // motor is connected backwards so needs to be flipped here
                    .pm100_direction_command = pm100_ptr->reverse_mode_dangerous ?
                        PM100_DIRECTION_FORWARD :
                        PM100_DIRECTION_REVERSE,
                    .pm100_speed_mode_enable = PM100_SPEED_MODE_DISABLE,
                    .pm100_inverter_enable = inverter_enable
                };

                can_t_pm100_command_message_pack(msg.data, &cmd, msg.length);

                LOG_DEBUG("Sending torque request\n");
                rtcan_status_t rtcan_status =
                    rtcan_transmit(pm100_ptr->rtcan_t_ptr, &msg);
                status = (rtcan_status == RTCAN_OK) ? STATUS_OK : STATUS_ERROR;
            }
            else
            {
                // to get out of lockout, need to send a disable command
                if (!pm100_ptr->lockout_logged)
                {
                    LOG_WARN("Still in lockout at torque request\n");
                    pm100_ptr->lockout_logged = true;
                }
                status = pm100_disable(pm100_ptr);
            }
        }

        (void)xSemaphoreGive(pm100_ptr->state_mutex);
    }
    else
    {
        // the mutex may have been acquired even though the request can't be
        // sent (e.g. latched error), so make sure it is always released
        if (mutex_status == pdPASS)
        {
            (void)xSemaphoreGive(pm100_ptr->state_mutex);
        }

        // this runs every control tick while in a fault, so only log once
        if (!pm100_ptr->request_failed)
        {
            LOG_ERROR("Failed to send torque request\n");
        }
        pm100_ptr->request_failed = true;
        (void)pm100_disable(pm100_ptr); // just in case
        status = STATUS_ERROR;
    }

    return status;
}
