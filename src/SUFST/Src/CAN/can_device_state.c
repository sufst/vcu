/***************************************************************************
 * @file   can_device_state.c
 * @author Chua Shen Yik (@hashyaha, syc2e18@soton.ac.uk)
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-01-09
 * @brief  CAN message parser and device state updater implementation
 ***************************************************************************/

#include "can_device_state.h"

#include "gpio.h"

#include "can_rx_thread.h"
#include "drs.h"
#include "pm100.h"

#define CAN_STATUS_LED_Pin  YELLOW_LED_Pin
#define CAN_STATUS_LED_Port YELLOW_LED_GPIO_Port

/**
 * @brief       CAN parser function type
 *
 * @note        Must be followed by all CAN parser functions
 *
 * @param[in]   msg_ptr     Pointer to received CAN message
 * @param[in]   dict_index  CAN message type dictionary index
 */
typedef void (*can_parser_t)(const can_msg_t* msg_ptr, uint32_t dict_index);

/*
 * parser function prototypes
 */
static void parser_std_little_endian(const can_msg_t* msg_ptr,
                                     uint32_t dict_index);
static void parser_pm100_analog_voltage(const can_msg_t* msg_ptr,
                                        uint32_t dict_index);
static void parser_pm100_internal_status(const can_msg_t* msg_ptr,
                                         uint32_t dict_index);
static void parser_pm100_diagnostic(const can_msg_t* msg_ptr,
                                    uint32_t dict_index);
static void parser_drs_little_endian(const can_msg_t* msg_ptr,
                                     uint32_t dict_index);

/**
 * @brief 	CAN segment map types
 *
 * @details	Big endian format
 */
typedef struct segment_map_s
{
    uint32_t index;     // index in CAN inputs array
    uint8_t start_byte; // input start byte (MSB) in CAN data field
    uint8_t size;       // input size in bytes
} can_segment_map_t;

/*
 * segment maps for PM100 inverter
 */
can_segment_map_t PM100_TEMP1_map[] = {
    {PM100_MODULE_A_TEMP, 0, 2},
    {PM100_MODULE_B_TEMP, 2, 2},
    {PM100_MODULE_C_TEMP, 4, 2},
    {PM100_GATE_DRIVER_BOARD_TEMP, 6, 2},
};

can_segment_map_t PM100_TEMP2_map[] = {
    {PM100_CONTROL_BOARD_TEMPERATURE, 0, 2},
    {PM100_RTD_1_TEMP, 2, 2},
    {PM100_RTD_2_TEMP, 4, 2},
    {PM100_RTD_3_TEMP, 6, 2},
};

can_segment_map_t PM100_TEMP3_map[] = {
    {PM100_COOLANT_TEMP, 0, 2},
    {PM100_HOT_SPOT_TEMP, 2, 2},
    {PM100_MOTOR_TEMP, 4, 2},
    {PM100_TORQUE_SHUDDER, 6, 2},
};

can_segment_map_t PM100_DIGI_map[] = {
    {PM100_DIGITAL_INPUT_1, 0, 1},
    {PM100_DIGITAL_INPUT_2, 1, 1},
    {PM100_DIGITAL_INPUT_3, 2, 1},
    {PM100_DIGITAL_INPUT_4, 3, 1},
    {PM100_DIGITAL_INPUT_5, 4, 1},
    {PM100_DIGITAL_INPUT_6, 5, 1},
    {PM100_DIGITAL_INPUT_7, 6, 1},
    {PM100_DIGITAL_INPUT_8, 7, 1},
};

can_segment_map_t PM100_MOTORPOS_map[] = {
    {PM100_MOTOR_ANGLE_ELECTRICAL, 0, 2},
    {PM100_MOTOR_SPEED, 2, 2},
    {PM100_ELECTRICAL_OUTPUT_FREQ, 4, 2},
    {PM100_DELTA_RESOLVER_FILTERED, 6, 2},
};

can_segment_map_t PM100_CURRENTINF_map[] = {
    {PM100_PHASE_A_CURRENT, 0, 2},
    {PM100_PHASE_B_CURRENT, 2, 2},
    {PM100_PHASE_C_CURRENT, 4, 2},
    {PM100_DC_BUS_CURRENT, 6, 2},
};

can_segment_map_t PM100_VOLTINF_map[] = {
    {PM100_DC_BUS_VOLTAGE, 0, 2},
    {PM100_OUTPUT_VOLTAGE, 2, 2},
    {PM100_VAB_VD_VOLTAGE, 4, 2},
    {PM100_VBC_VQ_VOLTAGE, 6, 2},
};

can_segment_map_t PM100_FLUXINF_map[] = {
    {PM100_FLUX_COMMAND, 0, 2},
    {PM100_FLUX_FEEDBACK, 2, 2},
    {PM100_ID_FEEDBACK, 4, 2},
    {PM100_IQ_FEEDBACK, 6, 2},
};

can_segment_map_t PM100_INTVOLT_map[] = {
    {PM100_REFERENCE_VOLTAGE_1V5, 0, 2},
    {PM100_REFERENCE_VOLTAGE_2V5, 2, 2},
    {PM100_REFERENCE_VOLTAGE_5V, 4, 2},
    {PM100_REFERENCE_VOLTAGE_12V, 6, 2},
};

can_segment_map_t PM100_FAULTCODES_map[] = {
    {PM100_POST_FAULT_LO, 0, 2},
    {PM100_POST_FAULT_HI, 2, 2},
    {PM100_RUN_FAULT_LO, 4, 2},
    {PM100_RUN_FAULT_HI, 6, 2},
};

can_segment_map_t PM100_TORQTIM_map[] = {
    {PM100_COMMANDED_TORQUE, 0, 2},
    {PM100_TORQUE_FEEDBACK, 2, 2},
    {PM100_POWER_ON_TIMER, 4, 4},
};

can_segment_map_t PM100_MODFLUX_map[] = {
    {PM100_MODULATION_INDEX, 0, 2},
    {PM100_FLUX_WEAKENING_OUTPUT, 2, 2},
    {PM100_ID_COMMAND, 4, 2},
    {PM100_IQ_COMMAND, 6, 2},
};

can_segment_map_t PM100_FIRMINF_map[] = {
    {PM100_EEPROM_VERSION, 0, 2},
    {PM100_SOFTWARE_VERSION, 2, 2},
    {PM100_DATE_CODE_MD, 4, 2},
    {PM100_DATE_CODE_YY, 6, 2},
};

can_segment_map_t PM100_HIGHSPEED_map[] = {
    {PM100_FAST_COMMANDED_TORQUE, 0, 2},
    {PM100_FAST_TORQUE_FEEDBACK, 2, 2},
    {PM100_FAST_MOTOR_SPEED, 4, 2},
    {PM100_FAST_DC_BUS_VOLTAGE, 6, 2},
};

can_segment_map_t PM100_PARAMETER_RESPONSE_map[] = {
    {PM100_PARAMETER_RESPONSE_ADDRESS, 0, 2},
    {PM100_PARAMETER_RESPONSE_WRITE_SUCCESS, 2, 1},
    {PM100_PARAMETER_RESPONSE_DATA, 4, 2},
};

/**
 * @brief CAN message type dictionary entry
 */
typedef struct can_dict_entry_s
{
    /**
     * @brief Message ID
     */
    uint32_t msg_ID;

    /**
     * @brief Message type name
     */
    char name[20];

    /**
     * @brief Associated segment map for data
     */
    can_segment_map_t* segment_map;

    /**
     * @brief Number of segments
     */
    uint8_t num_inputs;

    /**
     * @brief Parser function pointer
     */
    can_parser_t parser;

    /**
     * @brief Device state setter function pointer
     */
    can_device_state_setter_t state_setter;

} can_dict_entry_t;

/*
 * CAN message types dictionary
 */
static can_dict_entry_t can_dict[] = {
    // PM100 messages
    {PM100_CAN_ID_OFFSET + 0x00,
     "Temp_1",
     PM100_TEMP1_map,
     4,
     parser_std_little_endian,
     pm100_update_state},
    {PM100_CAN_ID_OFFSET + 0x01,
     "Temp_2",
     PM100_TEMP2_map,
     4,
     parser_std_little_endian,
     pm100_update_state},
    {PM100_CAN_ID_OFFSET + 0x02,
     "Temp_3_Torq_Shud",
     PM100_TEMP3_map,
     4,
     parser_std_little_endian,
     pm100_update_state},
    {PM100_CAN_ID_OFFSET + 0x03,
     "Analog_Input_Volt",
     NULL,
     0,
     parser_pm100_analog_voltage,
     pm100_update_state},
    {PM100_CAN_ID_OFFSET + 0x04,
     "Digital_Input_Status",
     PM100_DIGI_map,
     8,
     parser_std_little_endian,
     pm100_update_state},
    {PM100_CAN_ID_OFFSET + 0x05,
     "Motor_Position_Info",
     PM100_MOTORPOS_map,
     4,
     parser_std_little_endian,
     pm100_update_state},
    {PM100_CAN_ID_OFFSET + 0x06,
     "Current_Info",
     PM100_CURRENTINF_map,
     4,
     parser_std_little_endian,
     pm100_update_state},
    {PM100_CAN_ID_OFFSET + 0x07,
     "Volt_Info",
     PM100_VOLTINF_map,
     4,
     parser_std_little_endian,
     pm100_update_state},
    {PM100_CAN_ID_OFFSET + 0x08,
     "Flux_Info",
     PM100_FLUXINF_map,
     4,
     parser_std_little_endian,
     pm100_update_state},
    {PM100_CAN_ID_OFFSET + 0x09,
     "Internal_Volt",
     PM100_INTVOLT_map,
     4,
     parser_std_little_endian,
     pm100_update_state},
    {PM100_CAN_ID_OFFSET + 0x0A,
     "Internal_States",
     NULL,
     0,
     parser_pm100_internal_status,
     pm100_update_state},
    {PM100_CAN_ID_OFFSET + 0x0B,
     "Fault_Codes",
     PM100_FAULTCODES_map,
     4,
     parser_std_little_endian,
     pm100_update_state},
    {PM100_CAN_ID_OFFSET + 0x0C,
     "Torque_Timer_Info",
     PM100_TORQTIM_map,
     3,
     parser_std_little_endian,
     pm100_update_state},
    {PM100_CAN_ID_OFFSET + 0x0D,
     "Mod_Idx_FluxWeak",
     PM100_MODFLUX_map,
     4,
     parser_std_little_endian,
     pm100_update_state},
    {PM100_CAN_ID_OFFSET + 0x0E,
     "Firm_Info",
     PM100_FIRMINF_map,
     4,
     parser_std_little_endian,
     pm100_update_state},
    {PM100_CAN_ID_OFFSET + 0x0F,
     "Diagnostic",
     NULL,
     0,
     parser_pm100_diagnostic,
     pm100_update_state},
    {PM100_CAN_ID_OFFSET + 0x10,
     "High_Speed_Message",
     PM100_HIGHSPEED_map,
     4,
     parser_std_little_endian,
     pm100_update_state},
    {PM100_CAN_ID_OFFSET + 0x22,
     "Parameter_Response",
     PM100_PARAMETER_RESPONSE_map,
     3,
     parser_std_little_endian,
     pm100_update_state},
    {DRS_CAN_ID,
     "DRS_Input",
     NULL,
     0,
     parser_drs_little_endian,
     drs_update_state},
};

/**
 * @brief   Parse CAN received message and update received data
 *
 * @param   message_ptr
 */
void can_update_device_state(const can_msg_t* message_ptr)
{
    // search through CAN dictionary for matching message ID
    uint32_t dict_index;
    const uint32_t dict_length = sizeof(can_dict) / sizeof(can_dict_entry_t);

    for (dict_index = 0; dict_index < dict_length; dict_index++)
    {
        // TODO: should this be StdId?? used to be 'Identifier' with FDCAN
        if (message_ptr->rx_header.StdId == can_dict[dict_index].msg_ID)
        {
            break;
        }
    }

    // run the parser to update CAN data
    can_dict[dict_index].parser(message_ptr, dict_index);
}

/**
 * @brief Standard parser for unpacking Rinehart messages (and other little
 * endian messages)
 */
static void parser_std_little_endian(const can_msg_t* msg_ptr,
                                     uint32_t dict_index)
{
    can_dict_entry_t* dict_entry = &can_dict[dict_index];

    // iterate over all inputs for message type
    for (int i = 0; i < dict_entry->num_inputs; i++)
    {
        uint32_t word = 0;
        can_segment_map_t* map = &dict_entry->segment_map[i];

        // iterate over all bytes of given input
        for (int j = map->size - 1; j >= 0; j--)
        {
            // reverse endianness of bytes to construct word
            uint32_t byte_index = map->start_byte + j;
            uint32_t byte = (uint32_t) msg_ptr->data[byte_index];
            word <<= 8;
            word |= byte;
        }

        // write to device state
        dict_entry->state_setter(map->index, word);
    }
}

/**
 * @brief Special parser for Rinehart PM100 analog input voltages broadcast
 * messages
 */
static void parser_pm100_analog_voltage(const can_msg_t* msg_ptr,
                                        uint32_t dict_index)
{
    can_dict_entry_t* dict_entry = &can_dict[dict_index];

    // store the input bytes in little endian in order (7 6 5 4 ...) instead of
    // (0 1 2 ...)
    uint32_t first32bits
        = (uint32_t) msg_ptr->data[3] << 24 | (uint32_t) msg_ptr->data[2] << 16
          | (uint32_t) msg_ptr->data[1] << 8 | (uint32_t) msg_ptr->data[0];

    uint32_t second32bits
        = (uint32_t) msg_ptr->data[7] << 24 | (uint32_t) msg_ptr->data[6] << 16
          | (uint32_t) msg_ptr->data[5] << 8 | (uint32_t) msg_ptr->data[4];

    // analog inputs 1-3 from byte 0:3
    for (int i = 0; i < 3; i++)
    {
        uint32_t temp = first32bits << 22; // 32-10
        dict_entry->state_setter(PM100_ANALOG_INPUT_1 + i, temp >> 22);
        first32bits = first32bits >> 10;
    }

    // analog inputs 4-7 from byte 4:7
    for (int i = 0; i < 3; i++)
    {
        uint32_t temp = second32bits << 22; // 32-10
        dict_entry->state_setter(PM100_ANALOG_INPUT_4 + i, temp >> 22);
        second32bits = second32bits >> 10;
    }
}

/**
 * @brief Special parser for Rinehart PM100 internal status broadcast messages
 */
static void parser_pm100_internal_status(const can_msg_t* msg_ptr,
                                         uint32_t dict_index)
{
    can_dict_entry_t* dict_entry = &can_dict[dict_index];

    dict_entry->state_setter(PM100_VSM_STATE,
                             (uint32_t) msg_ptr->data[1] << 8
                                 | (uint32_t) msg_ptr->data[0]);
    dict_entry->state_setter(PM100_INVERTER_STATE, (uint32_t) msg_ptr->data[2]);
    dict_entry->state_setter(PM100_RELAY_STATE, (uint32_t) msg_ptr->data[3]);
    dict_entry->state_setter(PM100_INVERTER_RUN_MODE,
                             (uint32_t) (1 & msg_ptr->data[4]));
    dict_entry->state_setter(PM100_INVERTER_ACTIVE_DISCHARGE_STATE,
                             (uint32_t) (0xE0 & msg_ptr->data[4]) >> 5);
    dict_entry->state_setter(PM100_INVERTER_COMMAND_MODE,
                             (uint32_t) msg_ptr->data[5]);
    dict_entry->state_setter(PM100_INVERTER_ENABLE_STATE,
                             (uint32_t) (1 & msg_ptr->data[6]));
    dict_entry->state_setter(PM100_INVERTER_ENABLE_LOCKOUT,
                             (uint32_t) (1 & (msg_ptr->data[6] >> 7)));
    dict_entry->state_setter(PM100_DIRECTION_COMMAND,
                             (uint32_t) (1 & (msg_ptr->data[7] >> 0)));
    dict_entry->state_setter(PM100_BMS_ACTIVE,
                             (uint32_t) (1 & (msg_ptr->data[7] >> 1)));
    dict_entry->state_setter(PM100_BMS_LIMITING_TORQUE,
                             (uint32_t) (1 & (msg_ptr->data[7] >> 2)));
}

/**
 * @brief Special parser for Rinehart PM100 diagnostic broadcast messages
 */
static void parser_pm100_diagnostic(const can_msg_t* msg_ptr,
                                    uint32_t dict_index)
{
    // reverse endianness to extract diagnostic data
    uint32_t diagnostic_lo
        = (uint32_t) msg_ptr->data[0] << 24 | (uint32_t) msg_ptr->data[1] << 16
          | (uint32_t) msg_ptr->data[2] << 8 | (uint32_t) msg_ptr->data[3];

    uint32_t diagnostic_hi
        = (uint32_t) msg_ptr->data[4] << 24 | (uint32_t) msg_ptr->data[5] << 16
          | (uint32_t) msg_ptr->data[6] << 8 | (uint32_t) msg_ptr->data[7];

    // write data to device state
    can_dict_entry_t* dict_entry = &can_dict[dict_index];
    dict_entry->state_setter(PM100_DIAGNOSTIC_DATA_LO, diagnostic_lo);
    dict_entry->state_setter(PM100_DIAGNOSTIC_DATA_HI, diagnostic_hi);
}

/**
 * @brief Simple parser for unpacking DRS message from Dash (little endian)
 */
static void parser_drs_little_endian(const can_msg_t* msg_ptr,
                                     uint32_t dict_index)
{
    can_dict_entry_t* dict_entry = &can_dict[dict_index];

    // iterate over first 4 bytes of the input
    uint32_t word = 0;
    for (int j = 4 - 1; j >= 0; j--)
    {
        // reverse endianness of bytes to construct word
        uint32_t byte_index = j;
        uint32_t byte = (uint32_t) msg_ptr->data[byte_index];
        word <<= 8;
        word |= byte;
    }
    // write to device state
    dict_entry->state_setter(0, word);
}