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
