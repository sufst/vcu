/***************************************************************************
 * @file   can_device_state.h
 * @author Chua Shen Yik (@hashyaha, syc2e18@soton.ac.uk)
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-01-09
 * @brief  ?
 ***************************************************************************/

#ifndef CAN_TYPES_H
#define CAN_TYPES_H

#include <stdint.h>
#include "fdcan.h"
#include "can_message.h"

#define STD				FDCAN_STANDARD_ID
#define EXT				FDCAN_EXTENDED_ID
#define CAN_ID_OFFSET 	0xA0

/** 
 * @brief 	CAN input enums
 * 
 * @details	PM100DZ inverter input params
 * 
 * @note 	!!PRESERVE ORDER!! if changing any of this, remember to change parsers
 */
typedef enum can_input_e
{
	// 0x0A0 – Temperatures #1
	MODULE_A_TEMP,
	MODULE_B_TEMP,
	MODULE_C_TEMP,
	GATE_DRIVER_BOARD_TEMP,
	// 0x0A1 – Temperatures #2
	CONTROL_BOARD_TEMPERATURE,
	RTD_1_TEMP, 
	RTD_2_TEMP,
	RTD_3_TEMP,
	//** 0x0A2 – Temperatures #3 & Torque Shudder 
	COOLANT_TEMP,
	HOT_SPOT_TEMP,
	MOTOR_TEMP,
	TORQUE_SHUDDER,
	// 0x0A3 – Analog Input Voltages (for firmware version 1995 and after)
	ANALOG_INPUT_1,
	ANALOG_INPUT_2,
	ANALOG_INPUT_3,
	ANALOG_INPUT_4,
	ANALOG_INPUT_5,
	ANALOG_INPUT_6,
	// 0x0A4 – Digital Input Status
	DIGITAL_INPUT_1,
	DIGITAL_INPUT_2,
	DIGITAL_INPUT_3,
	DIGITAL_INPUT_4,
	DIGITAL_INPUT_5,
	DIGITAL_INPUT_6,
	DIGITAL_INPUT_7,
	DIGITAL_INPUT_8,
	// 0x0A5 – Motor Position Information
	MOTOR_ANGLE_ELECTRICAL,
	MOTOR_SPEED,
	ELECTRICAL_OUTPUT_FREQ,
	DELTA_RESOLVER_FILTERED,
	// 0x0A6 – Current Information
	PHASE_A_CURRENT,
	PHASE_B_CURRENT,
	PHASE_C_CURRENT,
	DC_BUS_CURRENT,
	// 0x0A7 – Voltage Information
	DC_BUS_VOLTAGE,
	OUTPUT_VOLTAGE,
	VAB_VD_VOLTAGE,
	VBC_VQ_VOLTAGE,
	// 0xA8 – Flux Information
	FLUX_COMMAND,
	FLUX_FEEDBACK,
	ID_FEEDBACK,
	IQ_FEEDBACK,
	// 0x0A9 – Internal Voltages
	REFERENCE_VOLTAGE_1V5,
	REFERENCE_VOLTAGE_2V5,
	REFERENCE_VOLTAGE_5V,
	REFERENCE_VOLTAGE_12V,
	// 0x0AA – Internal States
	VSM_STATE,
	PWM_FREQ,
	INVERTER_STATE,
	RELAY_STATE,
	INVERTER_RUN_MODE,
	INVERTER_ACTIVE_DISCHARGE_STATE,
	INVERTER_COMMAND_MODE,
	ROLLING_COUNTER_VALUE,
	INVERTER_ENABLE_STATE,
	START_MODE_ACTIVE,
	INVERTER_ENABLE_LOCKOUT,
	DIRECTION_COMMAND,
	BMS_ACTIVE,
	BMS_LIMITING_TORQUE,
	LIMIT_MAX_SPEED,
	LIMIT_HOT_SPOT,
	LOW_SPEED_LIMITING,
	COOLANT_TEMP_LIMITING,
	// 0x0AB – Fault Codes
	POST_FAULT_LO,
	POST_FAULT_HI,
	RUN_FAULT_LO,
	RUN_FAULT_HI,
	// 0x0AC – Torque & Timer Information
	COMMANDED_TORQUE,
	TORQUE_FEEDBACK,
	POWER_ON_TIMER,
	// 0x0AD – Modulation Index & Flux Weakening Output Information
	MODULATION_INDEX,
	FLUX_WEAKENING_OUTPUT,
	ID_COMMAND,
	IQ_COMMAND,
	// 0x0AE – Firmware Information
	EEPROM_VERSION,
	SOFTWARE_VERSION,
	DATE_CODE_MD,
	DATE_CODE_YY,
	// 0x0AF – Diagnostic Data => see "Download Diagnostic Data" for details
	DIAGNOSTIC_DATA_LO,
	DIAGNOSTIC_DATA_HI,
	// 0x0B0 – High Speed Message (transmitted at 3ms, version 2042+)
	FAST_COMMANDED_TORQUE,
	FAST_TORQUE_FEEDBACK,
	FAST_MOTOR_SPEED,
	FAST_DC_BUS_VOLTAGE,
	// 0x0C2 - Read / Write Parameter Response – response from motor controller
	PARAMETER_RESPONSE_ADDRESS, 		// address stores the address of the parameter
	PARAMETER_RESPONSE_WRITE_SUCCESS, 	// the success or not byte
	PARAMETER_RESPONSE_DATA, 			// response data from motor controller

	NUM_INPUTS,

} can_input_t;

/** 
 * @brief       CAN device state setter function type
 * 
 * @note        Must be followed by all CAN device state setters
 * 
 * @param[in]   index 	 	Index of value (enum)
 * @param[in]   value		Value to set
 */
typedef void (*can_device_state_setter_t)(uint32_t index, uint32_t value);

/*
 * function prototypes
 */
void can_update_device_state(const can_msg_t* message_ptr);

#endif
