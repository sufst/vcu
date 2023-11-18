#include "pm100.h"

#include <can_c.h>

#define PM100_NO_FAULTS                    0x00

#define PM100_VSM_STATE_START              0x00
#define PM100_VSM_STATE_PRECHARGE_INIT     0x01
#define PM100_VSM_STATE_PRECHARGE_ACTIVE   0x02
#define PM100_VSM_STATE_PRECHARGE_COMPLETE 0x03
#define PM100_VSM_STATE_WAIT               0x04
#define PM100_VSM_STATE_READY              0x05
#define PM100_VSM_STATE_RUNNING            0x06
#define PM100_VSM_STATE_FAULT              0x07

#define PM100_LOCKOUT_DISABLED             0x0
#define PM100_LOCKOUT_ENABLED              0x1
#define PM100_CAN_MODE                     0x0
#define PM100_VSM_MODE                     0x1
#define PM100_DIRECTION_REVERSE            0x0
#define PM100_DIRECTION_FORWARD            0x1
#define PM100_INVERTER_OFF                 0x0
#define PM100_INVERTER_ON                  0x1
#define PM100_TORQUE_MODE                  0x0
#define PM100_SPEED_MODE                   0x1
#define PM100_SPEED_MODE_DISABLE           0x0
#define PM100_SPEED_MODE_ENABLE            0x1

#define PM100_DIRECTION_FORWARD            0x1
#define PM100_DIRECTION_REVERSE            0x0

static log_context_t* log_h;

/*
 * internal function prototypes
 */
static void pm100_thread_entry(ULONG input);
static void process_broadcast(pm100_context_t* pm100_ptr,
                              const rtcan_msg_t* msg_ptr);

/**
 * @brief   Initialises the PM100 service
 *
 * @param[in]   pm100_ptr       PM100 context
 * @param[in]   stack_pool_ptr  Memory pool for service thread stack
 * @param[in]   rtcan_ptr       RTCAN instance for receiving broadcasts
 * @param[in]   config_ptr      Configuration
 */
status_t pm100_init(pm100_context_t* pm100_ptr,
                    log_context_t* log_ptr,
                    TX_BYTE_POOL* stack_pool_ptr,
                    rtcan_handle_t* rtcan_ptr,
                    const config_pm100_t* config_ptr)
{
    pm100_ptr->config_ptr = config_ptr;
    pm100_ptr->rtcan_ptr = rtcan_ptr;
    pm100_ptr->error = PM100_ERROR_NONE;
    pm100_ptr->broadcasts_valid = false;
    log_h = log_ptr;

    status_t status = STATUS_OK;

    // create service thread
    void* stack_ptr = NULL;
    UINT tx_status = tx_byte_allocate(stack_pool_ptr,
                                      &stack_ptr,
                                      config_ptr->thread.stack_size,
                                      TX_NO_WAIT);

    if (tx_status == TX_SUCCESS)
    {
        tx_status = tx_thread_create(&pm100_ptr->thread,
                                     (CHAR*) config_ptr->thread.name,
                                     pm100_thread_entry,
                                     (ULONG) pm100_ptr,
                                     stack_ptr,
                                     config_ptr->thread.stack_size,
                                     config_ptr->thread.priority,
                                     config_ptr->thread.priority,
                                     TX_NO_TIME_SLICE,
                                     TX_AUTO_START);
    }

    // create CAN receive queue
    if (tx_status == TX_SUCCESS)
    {
        tx_status = tx_queue_create(&pm100_ptr->can_rx_queue,
                                    NULL,
                                    TX_1_ULONG,
                                    pm100_ptr->can_rx_queue_mem,
                                    sizeof(pm100_ptr->can_rx_queue_mem));
    }

    // create state mutex
    if (tx_status == TX_SUCCESS)
    {
        tx_status = tx_mutex_create(&pm100_ptr->state_mutex, NULL, TX_INHERIT);
    }

    if (tx_status != TX_SUCCESS)
    {
        status = STATUS_ERROR;
    }

    // check all ok
    if (status != STATUS_OK)
    {
        tx_thread_terminate(&pm100_ptr->thread);
    }

    // turn off power
    // TODO: rename 'status' pin
    HAL_GPIO_WritePin(STATUS_GPIO_Port, STATUS_Pin, GPIO_PIN_RESET);

    return status;
}

/**
 * @brief   PM100 service thread entry function
 *
 * @details This thread is responsible for receiving and processing broadcast
 *          messages from the PM100
 */
void pm100_thread_entry(ULONG input)
{
    pm100_context_t* pm100_ptr = (pm100_context_t*) input;
    const config_pm100_t* config_ptr = pm100_ptr->config_ptr;

    // set up RTCAN subscriptions
    uint32_t subscriptions[] = {CAN_C_PM100_INTERNAL_STATES_FRAME_ID,
                                CAN_C_PM100_FAULT_CODES_FRAME_ID};

    for (uint32_t i = 0; i < sizeof(subscriptions) / sizeof(subscriptions[0]);
         i++)
    {
        rtcan_status_t status = rtcan_subscribe(pm100_ptr->rtcan_ptr,
                                                subscriptions[i],
                                                &pm100_ptr->can_rx_queue);

        if (status != RTCAN_OK)
        {
            // TODO: update broadcast states with error
            tx_thread_terminate(&pm100_ptr->thread);
        }
    }

    // process incoming messages, or timeout
    while (1)
    {
        rtcan_msg_t* msg_ptr = NULL;
        UINT status = tx_queue_receive(&pm100_ptr->can_rx_queue,
                                       &msg_ptr,
                                       config_ptr->broadcast_timeout_ticks);

        if (status == TX_QUEUE_EMPTY)
        {
            // timed out
            // TODO: error
            pm100_ptr->broadcasts_valid = false;
        }
        else if (status == TX_SUCCESS && msg_ptr != NULL)
        {
            pm100_ptr->broadcasts_valid = true;
            process_broadcast(pm100_ptr, msg_ptr);
            rtcan_msg_consumed(pm100_ptr->rtcan_ptr, msg_ptr);
        }
        else
        {
            // TODO; some other error
        }
    }
}

/**
 * @brief   Processes incoming broadcast messages
 *
 * @param[in]   pm100_ptr   PM100 context
 * @param[in]   msg_ptr     Incoming message
 */
void process_broadcast(pm100_context_t* pm100_ptr, const rtcan_msg_t* msg_ptr)
{
    switch (msg_ptr->identifier)
    {
    case CAN_C_PM100_INTERNAL_STATES_FRAME_ID:
    {
        can_c_pm100_internal_states_unpack(&pm100_ptr->states,
                                           msg_ptr->data,
                                           msg_ptr->length);

        break;
    }

    case CAN_C_PM100_FAULT_CODES_FRAME_ID:
    {
        can_c_pm100_fault_codes_unpack(&pm100_ptr->faults,
                                       msg_ptr->data,
                                       msg_ptr->length);

        if (pm100_ptr->faults.pm100_run_fault_hi != PM100_NO_FAULTS
            || pm100_ptr->faults.pm100_post_fault_lo != PM100_NO_FAULTS)
        {
            pm100_ptr->error |= PM100_ERROR_RUN_FAULT;
            (void) pm100_disable(pm100_ptr);
        }
        else if (pm100_ptr->faults.pm100_post_fault_hi != PM100_NO_FAULTS
                 || pm100_ptr->faults.pm100_post_fault_lo != PM100_NO_FAULTS)
        {
            pm100_ptr->error |= PM100_ERROR_POST_FAULT;
            (void) pm100_disable(pm100_ptr);
        }

        break;
    }

    default:
        break;
    }
}

/**
 * @brief       Initiates the precharge sequence
 *
 * @details     When operating in CAN mode, the precharge sequence begins as
 *              soon as the PM100 receives power. There is no way to change
 *              this functionality, other than to disable the precharge function
 *              and implement a custom precharge controller.
 *
 *              This function enables an output pin which activates a relay that
 *              provides 12V power to the PM100. When the PM100 is powered, it
 *              starts broadcasting onto the CAN bus so the broadcast processing
 *              thread is also started.
 *
 * @param[in]   pm100_ptr   PM100 context
 */
status_t pm100_start_precharge(pm100_context_t* pm100_ptr)
{
    // TODO: change the name of the pin
    HAL_GPIO_WritePin(STATUS_GPIO_Port, STATUS_Pin, GPIO_PIN_SET);

    UINT status = tx_thread_resume(&pm100_ptr->thread);

    return (status == TX_SUCCESS) ? STATUS_OK : STATUS_ERROR;
}

/**
 * @brief       Checks if the PM100 is precharged
 *
 * @param[in]   pm100_ptr   PM100 context
 */
bool pm100_is_precharged(pm100_context_t* pm100_ptr)
{
    // TODO: consider adding a sensible timeout
    UINT tx_status = tx_mutex_get(&pm100_ptr->state_mutex, TX_WAIT_FOREVER);

    bool broadcasts_valid = false;
    uint8_t vsm_state = PM100_VSM_STATE_FAULT; // assume something safe

    if (tx_status == TX_SUCCESS)
    {
        vsm_state = pm100_ptr->states.pm100_vsm_state;
        broadcasts_valid = pm100_ptr->broadcasts_valid;
        (void) tx_mutex_put(&pm100_ptr->state_mutex);
    }

    return broadcasts_valid
           && (vsm_state == PM100_VSM_STATE_PRECHARGE_COMPLETE
               || vsm_state == PM100_VSM_STATE_WAIT
               || vsm_state == PM100_VSM_STATE_READY
               || vsm_state == PM100_VSM_STATE_RUNNING);
}

/**
 * @brief       Disables the inverter
 *
 * @details     A disable command has data all zero. A disable command must be
 *              sent before enabling the inverter to exit the lockout state.
 *
 * @param[in]   pm100_ptr
 */
status_t pm100_disable(pm100_context_t* pm100_ptr)
{
    LOG_INFO(log_h, "Sending PM100 Disable command\n");
    rtcan_msg_t msg = {.identifier = CAN_C_PM100_COMMAND_MESSAGE_FRAME_ID,
                       .length = CAN_C_PM100_COMMAND_MESSAGE_LENGTH,
                       .data = {0, 0, 0, 0, 0, 0, 0, 0}};

    rtcan_status_t status = rtcan_transmit(pm100_ptr->rtcan_ptr, &msg);

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
status_t pm100_request_torque(pm100_context_t* pm100_ptr, uint16_t torque)
{
    status_t status = STATUS_OK;
    const bool is_precharged = pm100_is_precharged(
        pm100_ptr); // do this before getting mutex to avoid deadlock
    const bool no_errors = (pm100_ptr->error == PM100_ERROR_NONE);

    // TODO: consider adding a sensible timeout
    UINT tx_status = tx_mutex_get(&pm100_ptr->state_mutex, TX_WAIT_FOREVER);

    if (no_errors && is_precharged && tx_status == TX_SUCCESS)
    {
        if (pm100_ptr->broadcasts_valid)
        {
            if (pm100_ptr->states.pm100_inverter_enable_lockout
                == PM100_LOCKOUT_DISABLED)
            {
                rtcan_msg_t msg
                    = {.identifier = CAN_C_PM100_COMMAND_MESSAGE_FRAME_ID,
                       .length = CAN_C_PM100_COMMAND_MESSAGE_LENGTH,
                       .data = {0, 0, 0, 0, 0, 0, 0, 0}};

                struct can_c_pm100_command_message_t cmd
                    = {.pm100_torque_command = torque,
                       .pm100_direction_command = PM100_DIRECTION_FORWARD,
                       .pm100_speed_mode_enable = PM100_SPEED_MODE_DISABLE,
                       .pm100_inverter_enable = PM100_INVERTER_ON};

                can_c_pm100_command_message_pack(msg.data, &cmd, msg.length);

                LOG_INFO(log_h, "Sending torque request\n");
                rtcan_status_t rtcan_status
                    = rtcan_transmit(pm100_ptr->rtcan_ptr, &msg);
                status = (rtcan_status == RTCAN_OK) ? STATUS_OK : STATUS_ERROR;
            }
            else
            {
                // to get out of lockout, need to send a disable command
                status = pm100_disable(pm100_ptr);
            }
        }

        (void) tx_mutex_put(&pm100_ptr->state_mutex);
    }
    else
    {
        (void) pm100_disable(pm100_ptr); // just in case
        status = STATUS_ERROR;
    }

    return status;
}