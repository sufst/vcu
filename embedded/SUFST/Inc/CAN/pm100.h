/***************************************************************************
 * @file   pm100.h
 * @author Chua Shen Yik (@hashyaha, syc2e18@soton.ac.uk)
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-01-09
 * @brief  PM100 inverter driver prototypes
 ***************************************************************************/

#ifndef PM100_H
#define PM100_H

#include <stdint.h>

/**
 * @brief Return codes
 */
typedef enum
{
	PM100_OK       = 0x00,
	PM100_ERROR    = 0x01,
	PM100_BUSY     = 0x02,
	PM100_TIMEOUT  = 0x03
} pm100_status_t;

/**
 * @brief PM100 inverter command message
 */
typedef struct
{
	uint16_t	torque_command;
	uint16_t	speed_command;
	uint8_t		direction;
	uint8_t		inverter_enable;
	uint8_t		inverter_discharge;
	uint8_t		speed_mode_enable;
	uint16_t	commanded_torque_limit;
} pm100_command_t;

/**
 * @brief 	PM100 device state array indexing enum
 * 
 * @details	Defines the index in the PM100 state array which
 * 			pm100_write_state() and pm100_read_state() write/read 
 * 			data
 */
typedef enum pm100_state_index_e
{
	// 0x0A0 – Temperatures #1
	PM100_MODULE_A_TEMP,
	PM100_MODULE_B_TEMP,
	PM100_MODULE_C_TEMP,
	PM100_GATE_DRIVER_BOARD_TEMP,
	// 0x0A1 – Temperatures #2
	PM100_CONTROL_BOARD_TEMPERATURE,
	PM100_RTD_1_TEMP, 
	PM100_RTD_2_TEMP,
	PM100_RTD_3_TEMP,
	//** 0x0A2 – Temperatures #3 & Torque Shudder 
	PM100_COOLANT_TEMP,
	PM100_HOT_SPOT_TEMP,
	PM100_MOTOR_TEMP,
	PM100_TORQUE_SHUDDER,
	// 0x0A3 – Analog Input Voltages (for firmware version 1995 and after)
	PM100_ANALOG_INPUT_1,
	PM100_ANALOG_INPUT_2,
	PM100_ANALOG_INPUT_3,
	PM100_ANALOG_INPUT_4,
	PM100_ANALOG_INPUT_5,
	PM100_ANALOG_INPUT_6,
	// 0x0A4 – Digital Input Status
	PM100_DIGITAL_INPUT_1,
	PM100_DIGITAL_INPUT_2,
	PM100_DIGITAL_INPUT_3,
	PM100_DIGITAL_INPUT_4,
	PM100_DIGITAL_INPUT_5,
	PM100_DIGITAL_INPUT_6,
	PM100_DIGITAL_INPUT_7,
	PM100_DIGITAL_INPUT_8,
	// 0x0A5 – Motor Position Information
	PM100_MOTOR_ANGLE_ELECTRICAL,
	PM100_MOTOR_SPEED,
	PM100_ELECTRICAL_OUTPUT_FREQ,
	PM100_DELTA_RESOLVER_FILTERED,
	// 0x0A6 – Current Information
	PM100_PHASE_A_CURRENT,
	PM100_PHASE_B_CURRENT,
	PM100_PHASE_C_CURRENT,
	PM100_DC_BUS_CURRENT,
	// 0x0A7 – Voltage Information
	PM100_DC_BUS_VOLTAGE,
	PM100_OUTPUT_VOLTAGE,
	PM100_VAB_VD_VOLTAGE,
	PM100_VBC_VQ_VOLTAGE,
	// 0xA8 – Flux Information
	PM100_FLUX_COMMAND,
	PM100_FLUX_FEEDBACK,
	PM100_ID_FEEDBACK,
	PM100_IQ_FEEDBACK,
	// 0x0A9 – Internal Voltages
	PM100_REFERENCE_VOLTAGE_1V5,
	PM100_REFERENCE_VOLTAGE_2V5,
	PM100_REFERENCE_VOLTAGE_5V,
	PM100_REFERENCE_VOLTAGE_12V,
	// 0x0AA – Internal States
	PM100_VSM_STATE,
	PM100_PWM_FREQ,
	PM100_INVERTER_STATE,
	PM100_RELAY_STATE,
	PM100_INVERTER_RUN_MODE,
	PM100_INVERTER_ACTIVE_DISCHARGE_STATE,
	PM100_INVERTER_COMMAND_MODE,
	PM100_ROLLING_COUNTER_VALUE,
	PM100_INVERTER_ENABLE_STATE,
	PM100_START_MODE_ACTIVE,
	PM100_INVERTER_ENABLE_LOCKOUT,
	PM100_DIRECTION_COMMAND,
	PM100_BMS_ACTIVE,
	PM100_BMS_LIMITING_TORQUE,
	PM100_LIMIT_MAX_SPEED,
	PM100_LIMIT_HOT_SPOT,
	PM100_LOW_SPEED_LIMITING,
	PM100_COOLANT_TEMP_LIMITING,
	// 0x0AB – Fault Codes
	PM100_POST_FAULT_LO,
	PM100_POST_FAULT_HI,
	PM100_RUN_FAULT_LO,
	PM100_RUN_FAULT_HI,
	// 0x0AC – Torque & Timer Information
	PM100_COMMANDED_TORQUE,
	PM100_TORQUE_FEEDBACK,
	PM100_POWER_ON_TIMER,
	// 0x0AD – Modulation Index & Flux Weakening Output Information
	PM100_MODULATION_INDEX,
	PM100_FLUX_WEAKENING_OUTPUT,
	PM100_ID_COMMAND,
	PM100_IQ_COMMAND,
	// 0x0AE – Firmware Information
	PM100_EEPROM_VERSION,
	PM100_SOFTWARE_VERSION,
	PM100_DATE_CODE_MD,
	PM100_DATE_CODE_YY,
	// 0x0AF – Diagnostic Data => see "Download Diagnostic Data" for details
	PM100_DIAGNOSTIC_DATA_LO,
	PM100_DIAGNOSTIC_DATA_HI,
	// 0x0B0 – High Speed Message (transmitted at 3ms, version 2042+)
	PM100_FAST_COMMANDED_TORQUE,
	PM100_FAST_TORQUE_FEEDBACK,
	PM100_FAST_MOTOR_SPEED,
	PM100_FAST_DC_BUS_VOLTAGE,
	// 0x0C2 - Read / Write Parameter Response – response from motor controller
	PM100_PARAMETER_RESPONSE_ADDRESS,
	PM100_PARAMETER_RESPONSE_WRITE_SUCCESS,
	PM100_PARAMETER_RESPONSE_DATA,
	PM100_NUM_STATES,
} pm100_state_index_t;

/*
 * function prototypes
 */
pm100_status_t pm100_init();
pm100_status_t pm100_command_tx(pm100_command_t* command_data);
pm100_status_t pm100_torque_request(uint32_t torque);
pm100_status_t pm100_speed_request(uint16_t speed);

pm100_status_t pm100_read_state(uint32_t index, uint32_t* value_ptr);
void pm100_update_state(uint32_t index, uint32_t value);

#endif
