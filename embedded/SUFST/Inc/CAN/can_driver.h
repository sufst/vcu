/***************************************************************************
 * @file   can_driver.h
 * @author Chua Shen Yik (@hashyaha, syc2e18@soton.ac.uk)
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-01-09
 * @brief  CAN driver prototypes and CAN broadcast signals database
 ***************************************************************************/

#ifndef CAN_TYPES_H
#define CAN_TYPES_H

#include <stdint.h>
#include "fdcan.h"

/* 
 * definitions
 */
#define STD							FDCAN_STANDARD_ID
#define EXT							FDCAN_EXTENDED_ID
#define CAN_ID_OFFSET 0xA0

/* Type Definitions ------------------------------------------------------------------------*/

/* enum for inputs vector index */
/* each broadcast usually has two bytes */

/** 
 * @brief 	CAN input enums
 * 
 * @details	PM100DZ inverter input params
 * 
 * @note 	!!PRESERVE ORDER!! if changing any of this, remember to change parsers
 */
typedef enum CAN_input_e
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

} CAN_input_t;

/**
 * @brief 	CAN segment mapping
 * 
 * @details	Big endian format
 */
typedef struct segment_map_s
{
	CAN_input_t 	index;			// index in CAN inputs array
	uint8_t 		start_byte;		// input start byte (MSB) in CAN data field
	uint8_t 		size;			// input size in bytes
} segment_map_t;

/**
 * @brief 	CAN message queue item
 */
typedef struct queue_msg_s
{
	union
	{
		FDCAN_TxHeaderTypeDef	Tx_header;
		FDCAN_RxHeaderTypeDef	Rx_header;
	};
	uint8_t data[8];
} queue_msg_t;

/** 
 * @brief CAN parser function prototype
 * 
 * @note  Must be followed by all CAN parsers 
 */
typedef void (*CAN_parser_t)(queue_msg_t q_msg, uint8_t CAN_idx);


/** 
 * @brief CAN message type map entry
 */
typedef struct CAN_msg_s
{
	uint32_t		msg_ID;			// Message ID
	uint32_t		msg_type;		// STD or EXT
	char 			name[20];		// internal message name
	segment_map_t*  segment_map;	// segment_map for data
	uint8_t			num_inputs;		// number of segments
	CAN_parser_t	parser;			// parser function

} CAN_msg_t;


/**
 * @brief CAN input state (export)
 */
extern volatile uint32_t CAN_inputs[NUM_INPUTS];

/*
 * function prototypes
 */
void CAN_Rx();
HAL_StatusTypeDef CAN_Send(queue_msg_t Tx_msg);
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs);

#endif
