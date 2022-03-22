/***************************************************************************
 * @file   pm100.c
 * @author Chua Shen Yik (syc2e18@soton.ac.uk)
 * @date   2022-01-09
 * @brief  Inverter CAN communication implementation
 ***************************************************************************/

#include "pm100.h"
#include "config.h"

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
void pm100_init(){

	/* TODO: Set up some start up values. E.g. activate/deactivate broadcast messages */

	pm100_eeprom_write_blocking(PM100_TIMEOUT_ADDR, PM100_TIMEOUT_VALUE);
}

/**
 * @brief 		Blocking write to the PM100 EEPROM
 *
 * @param[in] 	parameter_address 	the Parameter Address for the message
 * @param[in] 	data 				the data to send in bytes 4 and 5, should already be formatted in order [byte 4][byte 5] (formatting described in documentation)
 */
void pm100_eeprom_write_blocking(uint16_t parameter_address, uint16_t data)
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
		// TODO: HAL delay doesn't work under RTOS
		HAL_Delay(CAN_EEPROM_RETRY_DELAY);

		// check for success
		suc = CAN_inputs[PARAMETER_RESPONSE_WRITE_SUCCESS];
        res_ad = CAN_inputs[PARAMETER_RESPONSE_ADDRESS];
    }
}

/**
 * @brief 		Blocking read from the PM100 EEPROM
 *
 * @details		This updates the global CAN_inputs state
 *
 * @param[in]	parameter_address	Parameter address to read
 */
void pm100_eeprom_read_blocking(uint16_t parameter_address)
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
    	HAL_Delay(CAN_EEPROM_RETRY_DELAY);

    	// check for success
        res_data = CAN_inputs[PARAMETER_RESPONSE_DATA];
        res_ad = CAN_inputs[PARAMETER_RESPONSE_ADDRESS];
    }

    (void) res_data; // unused for now, keep for future use
}

/**
 * @brief 		Transmit a command to the inverter
 *
 * @param[in]	torque_command 			Torque request in [Nm times 10] (does parsing locally)
 * @param[in]	speed_command 			Angular speed request [RPM]
 * @param[in]	direction_command 		Direction - 1 or 0 (see documentation for use with brake regen)
 * @param[in]	inverter_enable 		Enable inverter - 1 or 0 (on / off)
 * @param[in]	inverter_discharge 		Enable inverter discharge - 1 or 0 (enable / disable)
 * @param[in]	speed_mode_enable 		0 = do not override mode, 1 = will change from torque to speed mode
 * @param[in]	commanded_torque_limit 	Maximum torque request [Nm times 10], if zero will default to parameter in EEPROM
 */
void pm100_command_tx(uint16_t torque_command, uint16_t speed_command, uint8_t direction_command, uint8_t inverter_enable, uint8_t inverter_discharge, uint8_t speed_mode_enable, uint16_t commanded_torque_limit)
{
	// construct message
	pm100_command_msg.data[0] = (torque_command & 0x00FF);
	pm100_command_msg.data[1] = ((torque_command & 0xFF00) >> 8);
	pm100_command_msg.data[2] = (speed_command & 0x00FF);
	pm100_command_msg.data[3] = ((speed_command & 0xFF00) >> 8);
	pm100_command_msg.data[4] = direction_command;
	pm100_command_msg.data[5] = inverter_enable;
	pm100_command_msg.data[5] |= inverter_discharge << 1;
	pm100_command_msg.data[5] |= speed_mode_enable << 2;
	pm100_command_msg.data[6] = (commanded_torque_limit & 0x00FF);
	pm100_command_msg.data[7] = ((commanded_torque_limit & 0xFF00) >> 8);

	// send message
	CAN_Send(pm100_command_msg);
}

/**
 * @brief 		Transmit torque request to inverter
 *
 * @details		Will disable lockout if lockout is enabled by sending an empty message
 *
 * @param[in]	torque	The torque request to send [Nm * 10]
 */
void pm100_torque_command_tx(UINT torque)
{
	// handle lockout
	if (CAN_inputs[INVERTER_ENABLE_LOCKOUT] == 1)
	{
		pm100_command_tx(0,0,0,0,0,0,0);
	}
	// transmit torque request
	else
	{
		pm100_command_tx((uint16_t) torque, 0, PM100_DIRECTION_COMMAND, 1, 0, 0, 0);
	}
}
