/***************************************************************************
 * @file   can_driver.c
 * @author Chua Shen Yik (@hashyaha, syc2e18@soton.ac.uk)
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-01-09
 * @brief  Implementation of CAN driver
 ***************************************************************************/

#include "can_driver.h"

#include <stdbool.h>

/* 
 * function prototypes
 */
static void CAN_parser_std_little_endian(queue_msg_t q_msg, uint8_t CAN_idx);
static void CAN_parser_ANALOGVOLT(queue_msg_t q_msg, uint8_t CAN_idx);
static void CAN_parser_INTST(queue_msg_t q_msg, uint8_t CAN_idx);
static void CAN_parser_DIAGNOSTIC(queue_msg_t q_msg, uint8_t CAN_idx);

/*
 * segment maps for PM100DZ inverter
 */
segment_map_t TEMP1_map[] =
{
    {MODULE_A_TEMP, 0, 2},
    {MODULE_B_TEMP, 2, 2},
    {MODULE_C_TEMP, 4, 2},
    {GATE_DRIVER_BOARD_TEMP, 6, 2},
};

segment_map_t TEMP2_map[] =
{
    {CONTROL_BOARD_TEMPERATURE, 0, 2},
    {RTD_1_TEMP, 2, 2},
    {RTD_2_TEMP, 4, 2},
    {RTD_3_TEMP, 6, 2},
};

segment_map_t TEMP3_map[] =
{
    {COOLANT_TEMP, 0, 2},
    {HOT_SPOT_TEMP, 2, 2},
    {MOTOR_TEMP, 4, 2},
    {TORQUE_SHUDDER, 6, 2},
};

segment_map_t DIGI_map[] =
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

segment_map_t MOTORPOS_map[] =
{
    {MOTOR_ANGLE_ELECTRICAL, 0, 2},
    {MOTOR_SPEED, 2, 2},
    {ELECTRICAL_OUTPUT_FREQ, 4, 2},
    {DELTA_RESOLVER_FILTERED, 6, 2},
};

segment_map_t CURRENTINF_map[] =
{
    {PHASE_A_CURRENT, 0, 2},
    {PHASE_B_CURRENT, 2, 2},
    {PHASE_C_CURRENT, 4, 2},
    {DC_BUS_CURRENT, 6, 2},
};

segment_map_t VOLTINF_map[] =
{
    {DC_BUS_VOLTAGE, 0, 2},
    {OUTPUT_VOLTAGE, 2, 2},
    {VAB_VD_VOLTAGE, 4, 2},
    {VBC_VQ_VOLTAGE, 6, 2},
};

segment_map_t FLUXINF_map[] =
{
    {FLUX_COMMAND, 0, 2},
    {FLUX_FEEDBACK, 2, 2},
    {ID_FEEDBACK, 4, 2},
    {IQ_FEEDBACK, 6, 2},
};

segment_map_t INTVOLT_map[] =
{
    {REFERENCE_VOLTAGE_1V5, 0, 2},
    {REFERENCE_VOLTAGE_2V5, 2, 2},
    {REFERENCE_VOLTAGE_5V, 4, 2},
    {REFERENCE_VOLTAGE_12V, 6, 2},
};

segment_map_t FAULTCODES_map[] =
{
    {POST_FAULT_LO, 0, 2},
    {POST_FAULT_HI, 2, 2},
    {RUN_FAULT_LO, 4, 2},
    {RUN_FAULT_HI, 6, 2},
};

segment_map_t TORQTIM_map[] =
{
    {COMMANDED_TORQUE, 0, 2},
    {TORQUE_FEEDBACK, 2, 2},
    {POWER_ON_TIMER, 4, 4},
};

segment_map_t MODFLUX_map[] =
{
    {MODULATION_INDEX, 0, 2},
    {FLUX_WEAKENING_OUTPUT, 2, 2},
    {ID_COMMAND, 4, 2},
    {IQ_COMMAND, 6, 2},
};

segment_map_t FIRMINF_map[] =
{
    {EEPROM_VERSION, 0, 2},
    {SOFTWARE_VERSION, 2, 2},
    {DATE_CODE_MD, 4, 2},
    {DATE_CODE_YY, 6, 2},
};

segment_map_t HIGHSPEED_map[] =
{
    {FAST_COMMANDED_TORQUE, 0, 2},
    {FAST_TORQUE_FEEDBACK, 2, 2},
    {FAST_MOTOR_SPEED, 4, 2},
    {FAST_DC_BUS_VOLTAGE, 6, 2},
};

segment_map_t PARAMETER_RESPONSE_map[] =
{
    {PARAMETER_RESPONSE_ADDRESS, 0, 2},
    {PARAMETER_RESPONSE_WRITE_SUCCESS, 2, 1},
    {PARAMETER_RESPONSE_DATA, 4, 2},
};

/*
 * CAN message dictionary
 */
static CAN_msg_t CAN_dict [] =
{
    // PM100 messages
    {CAN_ID_OFFSET+0x00, 	STD,	"Temp_1", 				TEMP1_map, 				4, 	    CAN_parser_std_little_endian},
    {CAN_ID_OFFSET+0x01, 	STD,	"Temp_2", 				TEMP2_map, 				4, 	    CAN_parser_std_little_endian},
    {CAN_ID_OFFSET+0x02, 	STD,	"Temp_3_Torq_Shud", 	TEMP3_map, 				4, 	    CAN_parser_std_little_endian},
    {CAN_ID_OFFSET+0x03, 	STD,	"Analog_Input_Volt", 	NULL, 					0, 	    CAN_parser_ANALOGVOLT},
    {CAN_ID_OFFSET+0x04, 	STD,	"Digital_Input_Status", DIGI_map, 				8, 	    CAN_parser_std_little_endian},
    {CAN_ID_OFFSET+0x05, 	STD,	"Motor_Position_Info",	MOTORPOS_map, 			4, 	    CAN_parser_std_little_endian},
    {CAN_ID_OFFSET+0x06, 	STD,	"Current_Info", 		CURRENTINF_map, 		4, 	    CAN_parser_std_little_endian},
    {CAN_ID_OFFSET+0x07, 	STD,	"Volt_Info", 			VOLTINF_map, 			4, 	    CAN_parser_std_little_endian},
    {CAN_ID_OFFSET+0x08, 	STD,	"Flux_Info", 			FLUXINF_map, 			4, 	    CAN_parser_std_little_endian},
    {CAN_ID_OFFSET+0x09, 	STD,	"Internal_Volt", 		INTVOLT_map, 			4, 	    CAN_parser_std_little_endian},
    {CAN_ID_OFFSET+0x0A, 	STD,	"Internal_States", 		NULL, 					0, 	    CAN_parser_INTST},
    {CAN_ID_OFFSET+0x0B, 	STD,	"Fault_Codes", 			FAULTCODES_map, 		4,	    CAN_parser_std_little_endian},
    {CAN_ID_OFFSET+0x0C, 	STD,	"Torque_Timer_Info", 	TORQTIM_map, 			3, 	    CAN_parser_std_little_endian},
    {CAN_ID_OFFSET+0x0D, 	STD,	"Mod_Idx_FluxWeak",		MODFLUX_map, 			4, 	    CAN_parser_std_little_endian},
    {CAN_ID_OFFSET+0x0E, 	STD,	"Firm_Info", 			FIRMINF_map, 			4, 	    CAN_parser_std_little_endian},
    {CAN_ID_OFFSET+0x0F, 	STD,	"Diagnostic",			NULL, 					0, 	    CAN_parser_DIAGNOSTIC},
    {CAN_ID_OFFSET+0x10, 	STD,	"High_Speed_Message", 	HIGHSPEED_map, 			4, 	    CAN_parser_std_little_endian},
    {CAN_ID_OFFSET+0x22, 	STD,	"Parameter_Response", 	PARAMETER_RESPONSE_map,	3,	    CAN_parser_std_little_endian},
    // ID					Type	Name                    Map                     Inputs  Parser function
};

/* 
 * CAN inputs vector
 */
volatile uint32_t CAN_inputs[NUM_INPUTS];

/**
 * @brief 	Fetch and Parse a CAN message from CAN FIFO
 *
 * @details	Activated through HAL_FDCAN notification interrupt
 *          Parsed data is stored in CAN_inputs
 */
void CAN_Rx()
{
    // fetch received message
    FDCAN_RxHeaderTypeDef rx_header;
    uint8_t	rx_data[8];

    HAL_StatusTypeDef status = HAL_FDCAN_GetRxMessage(&hfdcan1, FDCAN_RX_FIFO0, &(rx_header), rx_data);

    if (status != HAL_OK)
    {
        // TODO(?): handle error
        return;
    }

    // add message to receive queue
    queue_msg_t rx_msg;
    rx_msg.Rx_header = rx_header;

    for (int i = 0; i < sizeof(rx_data); i++)
    {
        rx_msg.data[i] = rx_data[i];
    }

    // TODO(?) split task into receive and parse CAN messages from queue
    
    // search through CAN dictionary for matching message ID
    uint8_t i = 0;
    uint8_t CAN_dict_length = sizeof(CAN_dict) / sizeof(CAN_msg_t);

    while (i < CAN_dict_length)
    {
        // if ID matches dictionary entry, call parser and break loop
        if (rx_msg.Rx_header.Identifier == CAN_dict[i].msg_ID)
        {
            CAN_dict[i].parser(rx_msg, i);
            break;
        }
        i++;
    }

    // reactivate notification
    HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
}

/**
 * @brief 				Push CAN message to CAN bus
 * @param[in] Tx_msg: 	outgoing CAN message
 */
HAL_StatusTypeDef CAN_Send(queue_msg_t Tx_msg)
{
    // TODO(?) check that CAN message is valid
    return HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &Tx_msg.Tx_header, Tx_msg.data);
}

/**
 * @brief       Standard parser for unpacking Rinehart messages (and other little endian messages)
 * 
 * @details     Parsed data goes into CAN_inputs table 
 *              Data converted from little endian to big endian
 * 
 * @param[in]   q_msg       Incoming CAN message
 * @param[in]   CAN_idx     Index of message in CAN_dict with message metadata
 */
static void CAN_parser_std_little_endian(queue_msg_t q_msg, uint8_t CAN_idx)
{
    // iterate over all inputs in data field
    for (int i = 0; i < CAN_dict[CAN_idx].num_inputs; i++)
    {
        uint32_t result = 0;
        segment_map_t input = CAN_dict[CAN_idx].segment_map[i];

        // iterate over all bytes of input
        for (int j = input.size - 1; j >=0; j--)
        {
            result = result << 8 | (uint32_t) (q_msg.data[input.start_byte + j]);
        }

        // store result in CAN_inputs table
        CAN_inputs[input.index] = result;
    }
}


/**
 * @brief       Special parser for Rinehart PM100 analog input voltages broadcast messages
 * 
 * @param[in]   q_msg       Incoming CAN message
 * @param[in]   CAN_msg     Reference message from CAN_dict w/ message metadata
 */
static void CAN_parser_ANALOGVOLT(queue_msg_t q_msg, uint8_t CAN_idx){

    // store the inputs bytes in little endian in order (7 6 5 4 ...) instead of (0 1 2 ...)
    uint32_t first32bits =    (uint32_t) q_msg.data[3] << 24 
                            | (uint32_t) q_msg.data[2] << 16 
                            | (uint32_t) q_msg.data[1] << 8 
                            | (uint32_t) q_msg.data[0];

    uint32_t second32bits =   (uint32_t) q_msg.data[7] << 24 
                            | (uint32_t) q_msg.data[6] << 16 
                            | (uint32_t) q_msg.data[5] << 8 
                            | (uint32_t) q_msg.data[4];

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
 * @brief       Special parser for Rinehart PM100 internal status broadcast messages
 * 
 * @param[in]   q_msg       Incoming CAN message
 * @param[in]   CAN_msg     Reference message from CAN_dict w/ message metadata
 */
static void CAN_parser_INTST(queue_msg_t q_msg, uint8_t CAN_idx)
{
    CAN_inputs[VSM_STATE]                       = (uint32_t) q_msg.data[1] << 8 | (uint32_t) q_msg.data[0];
    CAN_inputs[INVERTER_STATE]                  = (uint32_t) q_msg.data[2];
    CAN_inputs[RELAY_STATE]                     = (uint32_t) q_msg.data[3];
    CAN_inputs[INVERTER_RUN_MODE]               = (uint32_t) (1 & q_msg.data[4]);
    CAN_inputs[INVERTER_ACTIVE_DISCHARGE_STATE] = (uint32_t) (0xE0 & q_msg.data[4]) >> 5;
    CAN_inputs[INVERTER_COMMAND_MODE]           = (uint32_t) q_msg.data[5];
    CAN_inputs[INVERTER_ENABLE_STATE]           = (uint32_t) (1 & q_msg.data[6]);
    CAN_inputs[INVERTER_ENABLE_LOCKOUT]         = (uint32_t) (1 & (q_msg.data[6] >> 7));
    CAN_inputs[DIRECTION_COMMAND]               = (uint32_t) (1 & (q_msg.data[7] >> 0));
    CAN_inputs[BMS_ACTIVE]                      = (uint32_t) (1 & (q_msg.data[7] >> 1));
    CAN_inputs[BMS_LIMITING_TORQUE]             = (uint32_t) (1 & (q_msg.data[7] >> 2));
}


/**
 * @brief   Parser for Rinehart PM100 diagnostic broadcast messages
 * 
 * @param[in]   q_msg       Incoming CAN message
 * @param[in]   CAN_msg     Reference message from CAN_dict w/ message metadata
 */
static void CAN_parser_DIAGNOSTIC(queue_msg_t q_msg, uint8_t CAN_idx){

    CAN_inputs[DIAGNOSTIC_DATA_LO] = (uint32_t) q_msg.data[0] << 24 
                                     | (uint32_t) q_msg.data[1] << 16 
                                     | (uint32_t) q_msg.data[2] << 8 
                                     | (uint32_t) q_msg.data[3];

    CAN_inputs[DIAGNOSTIC_DATA_HI] = (uint32_t) q_msg.data[4] << 24 
                                     | (uint32_t) q_msg.data[5] << 16 
                                     | (uint32_t) q_msg.data[6] << 8 
                                     | (uint32_t) q_msg.data[7];
}

/**
  * @brief      CAN Rx Fifo 0 message callback
  * 
  * @param[in]  hfdcan  FDCAN handle
  */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET)
    {
        CAN_Rx();
    }
    else 
    {
        Error_Handler();
    }
}

