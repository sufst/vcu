/***************************************************************************
 * @file   pm100.c
 * @author Chua Shen Yik (syc2e18@soton.ac.uk)
 * @date   2022-01-09
 * @brief  Inverter CAN communication implementation
 ***************************************************************************/

#include "pm100.h"
#include "config.h"

#include "can_message.h"
#include "can_device_state.h"
#include "rtc_time.h"
#include "tx_api.h"

#define FDCAN_HANDLE            hfdcan1

#define TIMEOUT_ADDR		    172
#define BROADCAST_LO_WORD_ADDR  148

#define DIRECTION_COMMAND		1										// 0: reverse, 1: forward
#define TIMEOUT			        (INVERTER_TORQUE_REQUEST_TIMEOUT / 3)	// divide ms timeout in config by three

#define BROADCAST_LO_WORD       (  (INVERTER_BROADCAST_TEMPERATURE_1     << 0)  \
                                 | (INVERTER_BROADCAST_TEMPERATURE_2     << 1)  \
                                 | (INVERTER_BROADCAST_TEMPERATURE_3     << 2)  \
                                 | (INVERTER_BROADCAST_ANALOG_INPUTS     << 3)  \
                                 | (INVERTER_BROADCAST_DIGITAL_INPUTS    << 4)  \
                                 | (INVERTER_BROADCAST_MOTOR_POSITION    << 5)  \
                                 | (INVERTER_BROADCAST_CURRENTS          << 6)  \
                                 | (INVERTER_BROADCAST_VOLTAGES          << 7)  \
                                 | (INVERTER_BROADCAST_FLUX           	 << 8)  \
                                 | (INVERTER_BROADCAST_INTERNAL_VOLTAGES << 9)  \
                                 | (INVERTER_BROADCAST_INTERNAL_STATES   << 10) \
                                 | (INVERTER_BROADCAST_FAULT_CODES       << 11) \
                                 | (INVERTER_BROADCAST_TORQUE_TIMER	     << 12) \
                                 | (INVERTER_BROADCAST_MODULATION_FLUX   << 13) \
                                 | (INVERTER_BROADCAST_FIRMWARE_INFO     << 14) \
                                 | (INVERTER_BROADCAST_DIAGNOSTIC_DATA   << 15) )

#define STATE_MUTEX_NAME        "PM100 State Mutex"

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
static can_msg_t pm100_command_msg =
{
    .tx_header =
    {
            CAN_ID_OFFSET + 0x020,
            FDCAN_STANDARD_ID,
            FDCAN_DATA_FRAME,
            FDCAN_DLC_BYTES_8,
            FDCAN_ESI_ACTIVE,
            FDCAN_BRS_OFF,
            FDCAN_CLASSIC_CAN,
            FDCAN_NO_TX_EVENTS,
            0
    },
    .data = {0, 0, 0, 0, 0, 0, 0, 0}
};

/**
 * @brief PM100 parameter write message
 */
static can_msg_t pm100_parameter_write_msg =
{
    .tx_header =
    {
            CAN_ID_OFFSET + 0x021,
            FDCAN_STANDARD_ID,
            FDCAN_DATA_FRAME,
            FDCAN_DLC_BYTES_8,
            FDCAN_ESI_ACTIVE,
            FDCAN_BRS_OFF,
            FDCAN_CLASSIC_CAN,
            FDCAN_NO_TX_EVENTS,
            0
    },
    .data = {0, 0, 1, 0, 0, 0, 0, 0} // byte 3 set to 1 for write
};

/**
 * @brief PM100 parameter read message
 */
static can_msg_t pm100_parameter_read_msg =
{
    .tx_header =
    {
            CAN_ID_OFFSET + 0x021,
            FDCAN_STANDARD_ID,
            FDCAN_DATA_FRAME,
            FDCAN_DLC_BYTES_8,
            FDCAN_ESI_ACTIVE,
            FDCAN_BRS_OFF,
            FDCAN_CLASSIC_CAN,
            FDCAN_NO_TX_EVENTS,
            0
    },
    .data = {0, 0, 0, 0, 0, 0, 0, 0}
};

/**
 * @brief Initialise PM100
 */
pm100_status_t pm100_init()
{
    pm100_status_t status = PM100_OK;
    // // configure broadcast messages
    // pm100_status_t status = pm100_eeprom_write_blocking((uint16_t) BROADCAST_LO_WORD_ADDR, (uint16_t) BROADCAST_LO_WORD);

    // // configure timeout
    // if (status == PM100_OK)
    // {
    //     status = pm100_eeprom_write_blocking(TIMEOUT_ADDR, TIMEOUT);
    // }

    // create state mutex
    if (status == PM100_OK)
    {
        status = (tx_mutex_create(&pm100_state_mutex, STATE_MUTEX_NAME, TX_NO_INHERIT) == TX_SUCCESS) ? PM100_OK : PM100_ERROR; 
    }

    // reset state
    memset(pm100_state, 0x0000, sizeof(pm100_state));

    return status;
}

/**
 * @brief 		Blocking write to the PM100 EEPROM
 *
 * @param[in] 	parameter_address 	Parameter address for message
 * @param[in] 	data 				Data to send in bytes 4 and 5
 * 
 * @note 		Data should already be formatted in order [byte 4][byte 5] 
 * 				(formatting is described in inverter documentation)
 */
pm100_status_t pm100_eeprom_write_blocking(uint16_t parameter_address, uint16_t data)
{
    // construct message
    pm100_parameter_write_msg.data[0] = (parameter_address & 0x00FF);
    pm100_parameter_write_msg.data[1] = ((parameter_address & 0xFF00) >> 8);
    pm100_parameter_write_msg.data[4] = (data & 0x00FF);        // low byte
    pm100_parameter_write_msg.data[5] = ((data & 0xFF00) >> 8); // high byte

    // loop until parameter set successfully or max retry attempts reached
    uint32_t suc = 0;
    uint16_t res_ad = 0;
    UINT attempts = 0;

    while ((res_ad != parameter_address || !suc)
      && (attempts < INVERTER_EEPROM_MAX_RETRY))
    {
        // transmit message
        HAL_FDCAN_AddMessageToTxFifoQ(&FDCAN_HANDLE, &pm100_parameter_write_msg.tx_header, pm100_parameter_write_msg.data);
        attempts++;

        // allow time for a response
        // -> have to use RTC for delay because EEPROM write happens on system initialisation
        //    before the scheduler starts
        // -> can't use HAL_Delay() because SysTick doesn't tick with RTOS
        rtc_delay(INVERTER_EEPROM_RETRY_DELAY);

        // check for success
        // suc = CAN_inputs[PARAMETER_RESPONSE_WRITE_SUCCESS];
        // res_ad = CAN_inputs[PARAMETER_RESPONSE_ADDRESS];
    }

    return PM100_OK;
}

/**
 * @brief 		Blocking read from the PM100 EEPROM
 *
 * @details		This updates the global CAN_inputs state
 *
 * @param[in]	parameter_address	Parameter address to read
 */
pm100_status_t pm100_eeprom_read_blocking(uint16_t parameter_address)
{
    // construct message
    pm100_parameter_read_msg.data[0] = (parameter_address & 0x00FF);
    pm100_parameter_read_msg.data[1] = ((parameter_address & 0xFF00) >> 8);

    // loop until parameter read successfully or max retry attempts reached
    uint32_t res_data = 0;
    uint16_t res_ad = 0;
    UINT attempts = 0;

    while (res_ad != parameter_address
            && attempts < INVERTER_EEPROM_MAX_RETRY)
    {
        // transmit message
        HAL_FDCAN_AddMessageToTxFifoQ(&FDCAN_HANDLE, &pm100_parameter_read_msg.tx_header, pm100_parameter_read_msg.data);
        attempts++;

        // allow time for a response
        // -> have to use RTC for delay because EEPROM write happens on system initialisation
        //    before the scheduler starts
        // -> can't use HAL_Delay() because SysTick doesn't tick with RTOS
        rtc_delay(INVERTER_EEPROM_RETRY_DELAY);

        // check for success
        // TODO(@hashyaha,@t-bre) this will no longer work due to CAN Rx thread dispatch
        // res_data = CAN_inputs[PARAMETER_RESPONSE_DATA];
        // res_ad = CAN_inputs[PARAMETER_RESPONSE_ADDRESS];
    }

    (void) res_data; // unused for now, keep for future use
    return PM100_OK;
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
    pm100_command_msg.data[7] = ((command_data->commanded_torque_limit & 0xFF00) >> 8);

    // send message
    if (HAL_FDCAN_AddMessageToTxFifoQ(&FDCAN_HANDLE, &pm100_command_msg.tx_header, pm100_command_msg.data) != HAL_OK)
    {
        return PM100_ERROR;
    }
  
    return PM100_OK;
}

/**
 * @brief 		Transmit torque request to inverter
 *
 * @details		Will disable lockout if lockout is enabled by sending an empty message
 *
 * @param[in]	torque	The torque request to send [Nm * 10]
 */
pm100_status_t pm100_torque_command_tx(uint32_t torque)
{
    // initialise command data to 0
    pm100_command_t pm100_cmd = {0};
    pm100_status_t status;

    // handle lockout
    uint32_t lockout;
    pm100_read_state(PM100_INVERTER_ENABLE_LOCKOUT, &lockout);

    if (lockout == 1)
    {
        status = pm100_command_tx(&pm100_cmd);
    }
    // transmit torque request
    else
    {
        pm100_cmd.direction = DIRECTION_COMMAND;
        pm100_cmd.torque_command = (uint16_t) torque;
        pm100_cmd.inverter_enable = 1;
        status = pm100_command_tx(&pm100_cmd);
    }
    return status;
}

#if INVERTER_SPEED_MODE
/**
 * @brief 		Transmit speed request to inverter
 *
 * @details		Will disable lockout if lockout is enabled by sending an empty message
 *
 * @param[in]	speed	The signed speed request to send [RPM]
 */
pm100_status_t pm100_speed_command_tx(UINT speed)
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
        pm100_cmd.direction = DIRECTION_COMMAND;
        pm100_cmd.speed_command = (uint16_t) speed;
        pm100_cmd.inverter_enable = 1;
        pm100_cmd.speed_mode_enable = 1;
        status = pm100_command_tx(&pm100_cmd);
    }
    return status;
}
#endif

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