/***************************************************************************
 * @file   pm100.c
 * @author Chua Shen Yik (syc2e18@soton.ac.uk)
 * @date   2022-01-09
 * @brief  Inverter CAN communication implementation
 ***************************************************************************/

#include "pm100.h"
#include "config.h"

#include "rtc_time.h"

/*
 * PM100 addresses
 */
#define PM100_TIMEOUT_ADDR		172

/*
 * other PM100 constants
 */
#define PM100_TIMEOUT_VALUE			(CAN_TORQUE_REQUEST_TIMEOUT / 3)	// divide ms timeout in config by three
#define PM100_DIRECTION_COMMAND		1									// 0: reverse, 1: forward


/**
 * @brief PM100 command message
 */
static queue_msg_t pm100_command_msg =
{
    .Tx_header =
    {
    		CAN_ID_OFFSET + 0x020,FDCAN_STANDARD_ID,FDCAN_DATA_FRAME,FDCAN_DLC_BYTES_8,FDCAN_ESI_ACTIVE,FDCAN_BRS_OFF,FDCAN_CLASSIC_CAN,FDCAN_NO_TX_EVENTS,0
    },
    .data = {0, 0, 0, 0, 0, 0, 0, 0}
};

/**
 * @brief PM100 parameter write message
 */
static queue_msg_t pm100_parameter_write_msg =
{
    .Tx_header =
    {
    		CAN_ID_OFFSET + 0x021,FDCAN_STANDARD_ID,FDCAN_DATA_FRAME,FDCAN_DLC_BYTES_8,FDCAN_ESI_ACTIVE,FDCAN_BRS_OFF,FDCAN_CLASSIC_CAN,FDCAN_NO_TX_EVENTS,0
    },
    .data = {0, 0, 1, 0, 0, 0, 0, 0} // byte 3 set to 1 for write
};

/**
 * @brief PM100 parameter read message
 */
static queue_msg_t pm100_parameter_read_msg =
{
    .Tx_header =
    {
    		CAN_ID_OFFSET + 0x021,FDCAN_STANDARD_ID,FDCAN_DATA_FRAME,FDCAN_DLC_BYTES_8,FDCAN_ESI_ACTIVE,FDCAN_BRS_OFF,FDCAN_CLASSIC_CAN,FDCAN_NO_TX_EVENTS,0
    },
    .data = {0, 0, 0, 0, 0, 0, 0, 0}
};

/**
 * @brief	Initialise PM100
 */
pm100_status_t pm100_init(){

	/* TODO: Set up some start up values. E.g. activate/deactivate broadcast messages */

	return pm100_eeprom_write_blocking(PM100_TIMEOUT_ADDR, PM100_TIMEOUT_VALUE);
}

/**
 * @brief 		Blocking write to the PM100 EEPROM
 *
 * @param[in] 	parameter_address 	the Parameter Address for the message
 * @param[in] 	data 				the data to send in bytes 4 and 5, should already be formatted in order [byte 4][byte 5] (formatting described in documentation)
 */
pm100_status_t pm100_eeprom_write_blocking(uint16_t parameter_address, uint16_t data)
{
	// construct message
	pm100_parameter_write_msg.data[0] = (parameter_address & 0x00FF);
	pm100_parameter_write_msg.data[1] = ((parameter_address & 0xFF00) >> 8);
	pm100_parameter_write_msg.data[5] = ((data & 0xFF00) >> 8);
	pm100_parameter_write_msg.data[4] = (data & 0x00FF);

	// loop until parameter set successfully or max retry attempts reached
	uint32_t suc = 0;
	uint16_t res_ad = 0;
	UINT attempts = 0;

	while ((res_ad != parameter_address || !suc)
	  && (attempts < CAN_EEPROM_MAX_RETRY))
	{
		// transmit message
		CAN_Send(pm100_parameter_write_msg);
		attempts++;

		// allow time for a response
		// -> have to use RTC for delay because EEPROM write happens on system initialisation
		//    before the scheduler starts
		// -> can't use HAL_Delay() because SysTick doesn't tick with RTOS
		rtc_delay(CAN_EEPROM_RETRY_DELAY);

		// check for success
		suc = CAN_inputs[PARAMETER_RESPONSE_WRITE_SUCCESS];
		res_ad = CAN_inputs[PARAMETER_RESPONSE_ADDRESS];
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
    		&& attempts < CAN_EEPROM_MAX_RETRY)
    {
    	// transmit message
    	CAN_Send(pm100_parameter_read_msg);
    	attempts++;

		// allow time for a response
		// -> have to use RTC for delay because EEPROM write happens on system initialisation
		//    before the scheduler starts
		// -> can't use HAL_Delay() because SysTick doesn't tick with RTOS
    	rtc_delay(CAN_EEPROM_RETRY_DELAY);

    	// check for success
        res_data = CAN_inputs[PARAMETER_RESPONSE_DATA];
        res_ad = CAN_inputs[PARAMETER_RESPONSE_ADDRESS];
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
	if (CAN_Send(pm100_command_msg) != HAL_OK)
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
pm100_status_t pm100_torque_command_tx(UINT torque)
{
	// initialise command data to 0
	pm100_command_t pm100_cmd = {0};
	pm100_status_t status;

  // handle lockout
	if(CAN_inputs[INVERTER_ENABLE_LOCKOUT] == 1)
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

#if RUN_SPEED_MODE
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
	if(CAN_inputs[INVERTER_ENABLE_LOCKOUT] == 1)
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