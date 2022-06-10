/***************************************************************************
 * @file   pm100.c
 * @author Chua Shen Yik (syc2e18@soton.ac.uk)
 * @date   2022-01-09
 * @brief  Inverter CAN communication implementation
 ***************************************************************************/

#include "pm100.h"

#include "config.h"
#include "tx_api.h"

#include "can_device_state.h"
#include "can_message.h"

#define PM100_FDCAN_HANDLE      hfdcan1

#define PM100_TIMEOUT_ADDR      172

#define PM100_DIRECTION_COMMAND 1 // 0: reverse, 1: forward
#define PM100_TIMEOUT                \
    (INVERTER_TORQUE_REQUEST_TIMEOUT \
     / 3) // divide ms timeout in config by three

#define PM100_STATE_MUTEX_NAME "PM100 State Mutex"

/**
 * @brief PM100 state
 */
static uint32_t pm100_state[PM100_NUM_STATES];

/**
 * @brief State mutex
 */
static TX_MUTEX pm100_state_mutex;

/**
 * @brief PM100 command message
 */
static can_msg_t pm100_command_msg = {.tx_header = {PM100_CAN_ID_OFFSET + 0x020,
                                                    FDCAN_STANDARD_ID,
                                                    FDCAN_DATA_FRAME,
                                                    FDCAN_DLC_BYTES_8,
                                                    FDCAN_ESI_ACTIVE,
                                                    FDCAN_BRS_OFF,
                                                    FDCAN_CLASSIC_CAN,
                                                    FDCAN_NO_TX_EVENTS,
                                                    0},
                                      .data = {0, 0, 0, 0, 0, 0, 0, 0}};

/**
 * @brief PM100 parameter write message
 */
static can_msg_t pm100_parameter_write_msg = {
    .tx_header = {PM100_CAN_ID_OFFSET + 0x021,
                  FDCAN_STANDARD_ID,
                  FDCAN_DATA_FRAME,
                  FDCAN_DLC_BYTES_8,
                  FDCAN_ESI_ACTIVE,
                  FDCAN_BRS_OFF,
                  FDCAN_CLASSIC_CAN,
                  FDCAN_NO_TX_EVENTS,
                  0},
    .data = {0, 0, 1, 0, 0, 0, 0, 0} // byte 3 set to 1 for write
};

/**
 * @brief PM100 parameter read message
 */
static can_msg_t pm100_parameter_read_msg
    = {.tx_header = {PM100_CAN_ID_OFFSET + 0x021,
                     FDCAN_STANDARD_ID,
                     FDCAN_DATA_FRAME,
                     FDCAN_DLC_BYTES_8,
                     FDCAN_ESI_ACTIVE,
                     FDCAN_BRS_OFF,
                     FDCAN_CLASSIC_CAN,
                     FDCAN_NO_TX_EVENTS,
                     0},
       .data = {0, 0, 0, 0, 0, 0, 0, 0}};

/**
 * @brief Initialise PM100
 */
pm100_status_t pm100_init()
{
    UNUSED(pm100_parameter_read_msg);
    UNUSED(pm100_parameter_write_msg);

    // create state mutex
    pm100_status_t status = PM100_OK;
    if (tx_mutex_create(&pm100_state_mutex,
                        PM100_STATE_MUTEX_NAME,
                        TX_NO_INHERIT)
        != TX_SUCCESS)
    {
        status = PM100_ERROR;
    }

    // reset state
    memset(pm100_state, 0x0000, sizeof(pm100_state));

    return status;
}

/**
 * @brief 		Transmit a command to the inverter
 *
 * @param[in]	command_data  PM100 command data struct
 */
pm100_status_t pm100_command_tx(pm100_command_t* command_data)
{
    // construct message
    pm100_command_msg.data[0] = (command_data->torque_command & 0x00FF);
    pm100_command_msg.data[1] = ((command_data->torque_command & 0xFF00) >> 8);
    pm100_command_msg.data[2] = (command_data->speed_command & 0x00FF);
    pm100_command_msg.data[3] = ((command_data->speed_command & 0xFF00) >> 8);
    pm100_command_msg.data[4] = command_data->direction;
    pm100_command_msg.data[5] = command_data->inverter_enable;
    pm100_command_msg.data[5] |= command_data->inverter_discharge << 1;
    pm100_command_msg.data[5] |= command_data->speed_mode_enable << 2;
    pm100_command_msg.data[6] = (command_data->commanded_torque_limit & 0x00FF);
    pm100_command_msg.data[7]
        = ((command_data->commanded_torque_limit & 0xFF00) >> 8);

    // send message
    if (HAL_FDCAN_AddMessageToTxFifoQ(&PM100_FDCAN_HANDLE,
                                      &pm100_command_msg.tx_header,
                                      pm100_command_msg.data)
        != HAL_OK)
    {
        return PM100_ERROR;
    }

    return PM100_OK;
}

/**
 * @brief 		Transmit torque request to inverter
 *
 * @details		Will disable lockout if lockout is enabled by sending an empty
 * message
 *
 * @param[in]	torque	The torque request to send [Nm * 10]
 */
pm100_status_t pm100_torque_request(uint32_t torque)
{
    // initialise command data to 0
    pm100_status_t status;

    // handle lockout
    uint32_t lockout;
    pm100_read_state(PM100_INVERTER_ENABLE_LOCKOUT, &lockout);

    if (lockout == 1)
    {
        status = pm100_disable();
    }
    // transmit torque request
    else
    {
        pm100_command_t pm100_cmd = {0};
        pm100_cmd.direction = PM100_DIRECTION_COMMAND;
        pm100_cmd.torque_command = (uint16_t) torque;
        pm100_cmd.inverter_enable = 1;
        status = pm100_command_tx(&pm100_cmd);
    }
    return status;
}

/**
 * @brief 		Transmit speed request to inverter
 *
 * @details		Will disable lockout if lockout is enabled by sending an empty
 * message
 *
 * @param[in]	speed	Speed request to send [RPM]
 */
pm100_status_t pm100_speed_request(uint16_t speed)
{
    // initialise command data to 0
    pm100_command_t pm100_cmd = {0};
    pm100_status_t status;

    // handle lockout
    uint32_t lockout;
    pm100_read_state(PM100_INVERTER_ENABLE_LOCKOUT, &lockout);

    if (lockout == 1)
    {
        pm100_cmd.speed_mode_enable = 1;
        status = pm100_command_tx(&pm100_cmd);
    }
    // transmit speed request
    else
    {
        pm100_cmd.direction = PM100_DIRECTION_COMMAND;
        pm100_cmd.speed_command = speed;
        pm100_cmd.inverter_enable = 1;
        pm100_cmd.speed_mode_enable = 1;
        status = pm100_command_tx(&pm100_cmd);
    }
    return status;
}

/**
 * @brief Send
 *
 * @return PM100_OK     Success
 * @return PM100_ERROR  Failure
 */
pm100_status_t pm100_disable()
{
    pm100_command_t pm100_cmd = {0};
    return pm100_command_tx(&pm100_cmd);
}

/**
 * @brief       Thread-safe PM100 state read
 *
 * @param[in]   index       Index of state variable to read
 * @param[out]  value_ptr   Pointer to location to store fetched data
 */
pm100_status_t pm100_read_state(uint32_t index, uint32_t* value_ptr)
{
    pm100_status_t status = (index < PM100_NUM_STATES) ? PM100_OK : PM100_ERROR;

    if (status == PM100_OK && value_ptr != NULL)
    {
        tx_mutex_get(&pm100_state_mutex, TX_WAIT_FOREVER);
        *value_ptr = pm100_state[index];
        tx_mutex_put(&pm100_state_mutex);
    }

    return status;
}

/**
 * @brief       Thread-safe PM100 state update
 *
 * @param[in]   index   Index of state variable to write to
 * @param[in]   value   Value to write to state
 */
void pm100_update_state(uint32_t index, uint32_t value)
{
    if (index < PM100_NUM_STATES)
    {
        if (tx_mutex_get(&pm100_state_mutex, TX_WAIT_FOREVER) == TX_SUCCESS)
        {
            pm100_state[index] = value;
            tx_mutex_put(&pm100_state_mutex);
        }
    }
}