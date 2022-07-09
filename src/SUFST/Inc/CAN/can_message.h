/***************************************************************************
 * @file   can_message.h
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-05-15
 * @brief  CAN message type
 ***************************************************************************/

#ifndef CAN_MESSAGE_H
#define CAN_MESSAGE_H

#include <stdint.h>

#include "fdcan.h"

/**
 * @brief 	CAN message type
 */
typedef struct can_msg_s
{
    /**
     * @brief Header
     */
    union
    {
        /**
         * @brief Transmit message header
         */
        FDCAN_TxHeaderTypeDef tx_header;

        /**
         * @brief Receive message header
         */
        FDCAN_RxHeaderTypeDef rx_header;
    };

    /**
     * @brief Data (8 bytes)
     */
    uint8_t data[8];

} can_msg_t;

#endif