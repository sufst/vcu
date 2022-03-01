
#include "pm100.h"

#define DIRECTION_COMMAND 1 // 1: Forward

/* pm100 Command Message*/
static queue_msg_t pm100_command_msg =
{
    .Tx_header =
    {
    		CAN_ID_OFFSET + 0x020,FDCAN_STANDARD_ID,FDCAN_DATA_FRAME,FDCAN_DLC_BYTES_8,FDCAN_ESI_ACTIVE,FDCAN_BRS_OFF,FDCAN_CLASSIC_CAN,FDCAN_NO_TX_EVENTS,0
    },
    .data = {0, 0, 0, 0, 0, 0, 0, 0}
};

/* pm100 parameter command */
static queue_msg_t pm100_parameter_write_msg =
{
    .Tx_header =
    {
    		CAN_ID_OFFSET + 0x021,FDCAN_STANDARD_ID,FDCAN_DATA_FRAME,FDCAN_DLC_BYTES_8,FDCAN_ESI_ACTIVE,FDCAN_BRS_OFF,FDCAN_CLASSIC_CAN,FDCAN_NO_TX_EVENTS,0
    },
    .data = {0, 0, 1, 0, 0, 0, 0, 0} // byte 3 set to 1 for write
};

static queue_msg_t pm100_parameter_read_msg =
{
    .Tx_header =
    {
    		CAN_ID_OFFSET + 0x021,FDCAN_STANDARD_ID,FDCAN_DATA_FRAME,FDCAN_DLC_BYTES_8,FDCAN_ESI_ACTIVE,FDCAN_BRS_OFF,FDCAN_CLASSIC_CAN,FDCAN_NO_TX_EVENTS,0
    },
    .data = {0, 0, 0, 0, 0, 0, 0, 0}
};

/**
 * @brief Configure inverter
 *
 */
void pm100_init(){
	/* TODO: Set up some start up values. E.g. activate/deactivate broadcast messages */
	/* EEPROM addresses */
	uint16_t INVERTER_RUN_MODE_ADDR = 0x8E;
	uint16_t INVERTER_COMMAND_MODE_ADDR = 0x8F;
	uint16_t CAN_TERM_RESISTOR_PRESENT_ADDR = 0x91;
	uint16_t RAW_CAN_TERM_RESISTOR_PRESENT_ADDR = 0x11e;
	uint16_t CAN_COMMAND_MESSAGE_ACTIVE_ADDR = 0x92;
	uint16_t CAN_BIT_RATE_ADDR = 0x93;
	uint16_t CAN_ACTIVE_MESSAGES_LO_WORD_ADDR = 0x94;
	uint16_t CAN_DIAGNOSTIC_DATA_TRANSMIT_ACTIVE_ADDR = 0x9E;
	uint16_t CAN_INVERTER_ENABLE_SWITCH_ACTIVE_ADDR = 0x9F;
	uint16_t CAN_TIMEOUT_ADDR = 0xAC;
	parameter_write_command(INVERTER_RUN_MODE_ADDR, 0);
	parameter_write_command(INVERTER_COMMAND_MODE_ADDR, 0);
	parameter_write_command(CAN_TERM_RESISTOR_PRESENT_ADDR, 1);
	parameter_write_command(RAW_CAN_TERM_RESISTOR_PRESENT_ADDR, 1);
	parameter_write_command(CAN_COMMAND_MESSAGE_ACTIVE_ADDR, 1);
	parameter_write_command(CAN_BIT_RATE_ADDR, 1000);
	parameter_write_command(CAN_ACTIVE_MESSAGES_LO_WORD_ADDR, 0xFF); //enable all messages
	parameter_write_command(CAN_DIAGNOSTIC_DATA_TRANSMIT_ACTIVE_ADDR, 1);
	parameter_write_command(CAN_INVERTER_ENABLE_SWITCH_ACTIVE_ADDR, 0);
	parameter_write_command(CAN_TIMEOUT_ADDR, 3); // 3x333ms == 999ms timeout
}

/**
 * @brief a way to send parameter write messages to inverter
 * @param parameter_address the Parameter Address for the message
 * @param data the data to send in bytes 4 and 5, should already be formatted in order [byte 4][byte 5] (formatting described in documentation)
 */
void parameter_write_command(uint16_t parameter_address, uint16_t data)
{

	pm100_parameter_write_msg.data[0] = (parameter_address & 0x00FF);
	pm100_parameter_write_msg.data[1] = ((parameter_address & 0xFF00) >> 8);

	pm100_parameter_write_msg.data[5] = ((data & 0xFF00) >> 8);
	pm100_parameter_write_msg.data[4] = (data & 0x00FF);
	CAN_Send(pm100_parameter_write_msg);
	uint32_t suc = 0;
	// Wait until write success
    while(!suc){
        suc = CAN_inputs[PARAMETER_RESPONSE_WRITE_SUCCESS];
        uint32_t res_ad = CAN_inputs[PARAMETER_RESPONSE_ADDRESS];
        // printf("Wrtie Addr: %u, Response Addess: %lu, Success: %lu\n",parameter_address,res_ad,suc);
        HAL_Delay(100);
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
        CAN_Send(pm100_parameter_write_msg);
    }
}

/**
 * @brief a way to send parameter read messages to rinehart 1
 * @param parameter_address the Parameter Address for the message
 *
 */
void parameter_read_command(uint16_t parameter_address)
{

	pm100_parameter_read_msg.data[0] = (parameter_address & 0x00FF);
	pm100_parameter_read_msg.data[1] = ((parameter_address & 0xFF00) >> 8);


	CAN_Send(pm100_parameter_read_msg);
}


/**
 * @brief a way to send command messages to inverter
 * @param torque_command the torque command to send in N*m times 10 (does parsing locally)
 * @param speed_command the angular speed to send in RPM
 * @param direction_command either one or zero (see documentation for use with brake regen)
 * @param inverter_enable either 1 or 0 (inverter on or off)
 * @param inverter_discharge either 1 or 0 (enable discharge or disable discharge)
 * @param speed_mode_enable 0= do not override mode 1= will change from torque to speed mode (see manual "using speed mode")
 * @param commanded_torque_limit the max torque limit in N*m times 10, if zero will default to parameter in EEPROM
 */
void command_msg(uint16_t torque_command, uint16_t speed_command, uint8_t direction_command, uint8_t inverter_enable, uint8_t inverter_discharge, uint8_t speed_mode_enable, uint16_t commanded_torque_limit){
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

	CAN_Send(pm100_command_msg);
}


/**
 * @brief a way to send torque messages to inverter (will disable lockout if lockout is enabled by sending empty message)
 * @param torque_command the torque command to send in N*m times 10 (does parsing locally)
 */
void pm100_torque_command(uint16_t torque_command){

	if(CAN_inputs[INVERTER_ENABLE_LOCKOUT] == 1){
		command_msg(0,0,0,0,0,0,0);
	}
	else{
		command_msg(torque_command, 0, DIRECTION_COMMAND, 1, 0, 0, 0);
	}

}
