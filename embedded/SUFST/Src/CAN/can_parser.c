/***************************************************************************
 * @file   can_driver.c
 * @author Chua Shen Yik (@hashyaha, syc2e18@soton.ac.uk)
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-01-09
 * @brief  Implementation of CAN driver
 ***************************************************************************/

#include "can_parser.h"

#include "gpio.h"
#include "can_rx_thread.h"

#include <stdbool.h>

#define CAN_STATUS_LED_Pin      YELLOW_LED_Pin
#define CAN_STATUS_LED_Port     YELLOW_LED_GPIO_Port

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
static void parser_std_little_endian(const can_msg_t* msg_ptr, uint32_t dict_index);
static void parser_pm100_analog_voltage(const can_msg_t* msg_ptr, uint32_t dict_index);
static void parser_pm100_internal_status(const can_msg_t* msg_ptr, uint32_t dict_index);
static void parser_pm100_diagnostic(const can_msg_t* msg_ptr, uint32_t dict_index);

/**
 * @brief 	CAN segment map types
 * 
 * @details	Big endian format
 */
typedef struct segment_map_s
{
	can_input_t 	index;			// index in CAN inputs array
	uint8_t 		start_byte;		// input start byte (MSB) in CAN data field
	uint8_t 		size;			// input size in bytes
} can_segment_map_t;

/*
 * segment maps for PM100 inverter
 */
can_segment_map_t TEMP1_map[] =
{
    {MODULE_A_TEMP, 0, 2},
    {MODULE_B_TEMP, 2, 2},
    {MODULE_C_TEMP, 4, 2},
    {GATE_DRIVER_BOARD_TEMP, 6, 2},
};

can_segment_map_t TEMP2_map[] =
{
    {CONTROL_BOARD_TEMPERATURE, 0, 2},
    {RTD_1_TEMP, 2, 2},
    {RTD_2_TEMP, 4, 2},
    {RTD_3_TEMP, 6, 2},
};

can_segment_map_t TEMP3_map[] =
{
    {COOLANT_TEMP, 0, 2},
    {HOT_SPOT_TEMP, 2, 2},
    {MOTOR_TEMP, 4, 2},
    {TORQUE_SHUDDER, 6, 2},
};

can_segment_map_t DIGI_map[] =
{
    {DIGITAL_INPUT_1, 0, 1},
    {DIGITAL_INPUT_2, 1, 1},
    {DIGITAL_INPUT_3, 2, 1},
    {DIGITAL_INPUT_4, 3, 1},
    {DIGITAL_INPUT_5, 4, 1},
    {DIGITAL_INPUT_6, 5, 1},
    {DIGITAL_INPUT_7, 6, 1},
    {DIGITAL_INPUT_8, 7, 1},
};

can_segment_map_t MOTORPOS_map[] =
{
    {MOTOR_ANGLE_ELECTRICAL, 0, 2},
    {MOTOR_SPEED, 2, 2},
    {ELECTRICAL_OUTPUT_FREQ, 4, 2},
    {DELTA_RESOLVER_FILTERED, 6, 2},
};

can_segment_map_t CURRENTINF_map[] =
{
    {PHASE_A_CURRENT, 0, 2},
    {PHASE_B_CURRENT, 2, 2},
    {PHASE_C_CURRENT, 4, 2},
    {DC_BUS_CURRENT, 6, 2},
};

can_segment_map_t VOLTINF_map[] =
{
    {DC_BUS_VOLTAGE, 0, 2},
    {OUTPUT_VOLTAGE, 2, 2},
    {VAB_VD_VOLTAGE, 4, 2},
    {VBC_VQ_VOLTAGE, 6, 2},
};

can_segment_map_t FLUXINF_map[] =
{
    {FLUX_COMMAND, 0, 2},
    {FLUX_FEEDBACK, 2, 2},
    {ID_FEEDBACK, 4, 2},
    {IQ_FEEDBACK, 6, 2},
};

can_segment_map_t INTVOLT_map[] =
{
    {REFERENCE_VOLTAGE_1V5, 0, 2},
    {REFERENCE_VOLTAGE_2V5, 2, 2},
    {REFERENCE_VOLTAGE_5V, 4, 2},
    {REFERENCE_VOLTAGE_12V, 6, 2},
};

can_segment_map_t FAULTCODES_map[] =
{
    {POST_FAULT_LO, 0, 2},
    {POST_FAULT_HI, 2, 2},
    {RUN_FAULT_LO, 4, 2},
    {RUN_FAULT_HI, 6, 2},
};

can_segment_map_t TORQTIM_map[] =
{
    {COMMANDED_TORQUE, 0, 2},
    {TORQUE_FEEDBACK, 2, 2},
    {POWER_ON_TIMER, 4, 4},
};

can_segment_map_t MODFLUX_map[] =
{
    {MODULATION_INDEX, 0, 2},
    {FLUX_WEAKENING_OUTPUT, 2, 2},
    {ID_COMMAND, 4, 2},
    {IQ_COMMAND, 6, 2},
};

can_segment_map_t FIRMINF_map[] =
{
    {EEPROM_VERSION, 0, 2},
    {SOFTWARE_VERSION, 2, 2},
    {DATE_CODE_MD, 4, 2},
    {DATE_CODE_YY, 6, 2},
};

can_segment_map_t HIGHSPEED_map[] =
{
    {FAST_COMMANDED_TORQUE, 0, 2},
    {FAST_TORQUE_FEEDBACK, 2, 2},
    {FAST_MOTOR_SPEED, 4, 2},
    {FAST_DC_BUS_VOLTAGE, 6, 2},
};

can_segment_map_t PARAMETER_RESPONSE_map[] =
{
    {PARAMETER_RESPONSE_ADDRESS, 0, 2},
    {PARAMETER_RESPONSE_WRITE_SUCCESS, 2, 1},
    {PARAMETER_RESPONSE_DATA, 4, 2},
};

/** 
 * @brief CAN message type dictionary entry
 */
typedef struct can_dict_entry_s
{
    /**
     * @brief Message ID
     */
	uint32_t		    msg_ID;

    /**
     * @brief Message type (STD or EXT)
     */
	uint32_t		    msg_type;

    /**
     * @brief Message type name
     */
	char 			    name[20];

    /**
     * @brief Associated segment map for data
     */
	can_segment_map_t*  segment_map;

    /**
     * @brief Number of segments
     */
	uint8_t			    num_inputs;

    /**
     * @brief Parser function pointer
     */
	can_parser_t	    parser;

} can_dict_entry_t;

/*
 * CAN message dictionary
 */
static can_dict_entry_t can_dict[] =
{
    // PM100 messages
    {CAN_ID_OFFSET+0x00, 	STD,	"Temp_1", 				TEMP1_map, 				4, 	    parser_std_little_endian},
    {CAN_ID_OFFSET+0x01, 	STD,	"Temp_2", 				TEMP2_map, 				4, 	    parser_std_little_endian},
    {CAN_ID_OFFSET+0x02, 	STD,	"Temp_3_Torq_Shud", 	TEMP3_map, 				4, 	    parser_std_little_endian},
    {CAN_ID_OFFSET+0x03, 	STD,	"Analog_Input_Volt", 	NULL, 					0, 	    parser_pm100_analog_voltage},
    {CAN_ID_OFFSET+0x04, 	STD,	"Digital_Input_Status", DIGI_map, 				8, 	    parser_std_little_endian},
    {CAN_ID_OFFSET+0x05, 	STD,	"Motor_Position_Info",	MOTORPOS_map, 			4, 	    parser_std_little_endian},
    {CAN_ID_OFFSET+0x06, 	STD,	"Current_Info", 		CURRENTINF_map, 		4, 	    parser_std_little_endian},
    {CAN_ID_OFFSET+0x07, 	STD,	"Volt_Info", 			VOLTINF_map, 			4, 	    parser_std_little_endian},
    {CAN_ID_OFFSET+0x08, 	STD,	"Flux_Info", 			FLUXINF_map, 			4, 	    parser_std_little_endian},
    {CAN_ID_OFFSET+0x09, 	STD,	"Internal_Volt", 		INTVOLT_map, 			4, 	    parser_std_little_endian},
    {CAN_ID_OFFSET+0x0A, 	STD,	"Internal_States", 		NULL, 					0, 	    parser_pm100_internal_status},
    {CAN_ID_OFFSET+0x0B, 	STD,	"Fault_Codes", 			FAULTCODES_map, 		4,	    parser_std_little_endian},
    {CAN_ID_OFFSET+0x0C, 	STD,	"Torque_Timer_Info", 	TORQTIM_map, 			3, 	    parser_std_little_endian},
    {CAN_ID_OFFSET+0x0D, 	STD,	"Mod_Idx_FluxWeak",		MODFLUX_map, 			4, 	    parser_std_little_endian},
    {CAN_ID_OFFSET+0x0E, 	STD,	"Firm_Info", 			FIRMINF_map, 			4, 	    parser_std_little_endian},
    {CAN_ID_OFFSET+0x0F, 	STD,	"Diagnostic",			NULL, 					0, 	    parser_pm100_diagnostic},
    {CAN_ID_OFFSET+0x10, 	STD,	"High_Speed_Message", 	HIGHSPEED_map, 			4, 	    parser_std_little_endian},
    {CAN_ID_OFFSET+0x22, 	STD,	"Parameter_Response", 	PARAMETER_RESPONSE_map,	3,	    parser_std_little_endian},
    // ID					Type	Name                    Map                     Inputs  Parser function
};

/* 
 * CAN inputs vector
 */
volatile uint32_t CAN_inputs[NUM_INPUTS];

/**
 * @brief   Parse CAN received message and update received data
 * 
 * @param   message_ptr 
 */
void can_parse(const can_msg_t* message_ptr)
{
    // search through CAN dictionary for matching message ID
    uint32_t dict_index;
    const uint32_t dict_length = sizeof(can_dict) / sizeof(can_dict_entry_t);

    for (dict_index = 0; dict_index < dict_length; dict_index++)
    {
        if (message_ptr->rx_header.Identifier == can_dict[dict_index].msg_ID)
        {
            break;
        }
    }

    // run the parser to update CAN data
    can_dict[dict_index].parser(message_ptr, dict_index);
}

/**
 * @brief Standard parser for unpacking Rinehart messages (and other little endian messages)
 */
static void parser_std_little_endian(const can_msg_t* msg_ptr, uint32_t dict_index)
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

        // store result in CAN_inputs table
        CAN_inputs[map->index] = word;
    }
}

/**
 * @brief Special parser for Rinehart PM100 analog input voltages broadcast messages
 */
static void parser_pm100_analog_voltage(const can_msg_t* msg_ptr, uint32_t dict_index)
{
    // store the input bytes in little endian in order (7 6 5 4 ...) instead of (0 1 2 ...)
    uint32_t first32bits =    (uint32_t) msg_ptr->data[3] << 24 
                            | (uint32_t) msg_ptr->data[2] << 16 
                            | (uint32_t) msg_ptr->data[1] << 8 
                            | (uint32_t) msg_ptr->data[0];

    uint32_t second32bits =   (uint32_t) msg_ptr->data[7] << 24 
                            | (uint32_t) msg_ptr->data[6] << 16 
                            | (uint32_t) msg_ptr->data[5] << 8 
                            | (uint32_t) msg_ptr->data[4];

    // inputs 1-3 from byte 0:3
    for(int i = 0; i < 3; i++)
    {
        uint32_t temp = first32bits << 22; // 32-10
        CAN_inputs[ANALOG_INPUT_1 + i] = temp >> 22;
        first32bits = first32bits >> 10;
    }

    // inputs 4-7 from byte 4:7
    for(int i = 0; i < 3; i++)
    {
        uint32_t temp = second32bits << 22; // 32-10
        CAN_inputs[ANALOG_INPUT_4 + i] = temp >> 22;
        second32bits = second32bits >> 10;
    }
}

/**
 * @brief Special parser for Rinehart PM100 internal status broadcast messages
 */
static void parser_pm100_internal_status(const can_msg_t* msg_ptr, uint32_t dict_index)
{
    CAN_inputs[VSM_STATE]                       = (uint32_t) msg_ptr->data[1] << 8 | (uint32_t) msg_ptr->data[0];
    CAN_inputs[INVERTER_STATE]                  = (uint32_t) msg_ptr->data[2];
    CAN_inputs[RELAY_STATE]                     = (uint32_t) msg_ptr->data[3];
    CAN_inputs[INVERTER_RUN_MODE]               = (uint32_t) (1 & msg_ptr->data[4]);
    CAN_inputs[INVERTER_ACTIVE_DISCHARGE_STATE] = (uint32_t) (0xE0 & msg_ptr->data[4]) >> 5;
    CAN_inputs[INVERTER_COMMAND_MODE]           = (uint32_t) msg_ptr->data[5];
    CAN_inputs[INVERTER_ENABLE_STATE]           = (uint32_t) (1 & msg_ptr->data[6]);
    CAN_inputs[INVERTER_ENABLE_LOCKOUT]         = (uint32_t) (1 & (msg_ptr->data[6] >> 7));
    CAN_inputs[DIRECTION_COMMAND]               = (uint32_t) (1 & (msg_ptr->data[7] >> 0));
    CAN_inputs[BMS_ACTIVE]                      = (uint32_t) (1 & (msg_ptr->data[7] >> 1));
    CAN_inputs[BMS_LIMITING_TORQUE]             = (uint32_t) (1 & (msg_ptr->data[7] >> 2));
}


/**
 * @brief Special parser for Rinehart PM100 diagnostic broadcast messages
 */
static void parser_pm100_diagnostic(const can_msg_t* msg_ptr, uint32_t dict_index){

    CAN_inputs[DIAGNOSTIC_DATA_LO] = (uint32_t) msg_ptr->data[0] << 24 
                                     | (uint32_t) msg_ptr->data[1] << 16 
                                     | (uint32_t) msg_ptr->data[2] << 8 
                                     | (uint32_t) msg_ptr->data[3];

    CAN_inputs[DIAGNOSTIC_DATA_HI] = (uint32_t) msg_ptr->data[4] << 24 
                                     | (uint32_t) msg_ptr->data[5] << 16 
                                     | (uint32_t) msg_ptr->data[6] << 8 
                                     | (uint32_t) msg_ptr->data[7];
}