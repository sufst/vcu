/***************************************************************************
 * @file    logger.c
 * @author  Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief   Datalogger interface implementation
 ***************************************************************************/

#include "logger.h"

#include "can.h"
#include <memory.h>

typedef struct
{
    CAN_TxHeaderTypeDef header;
    uint8_t tx_data[8];
    uint32_t tx_mailbox;
} test_msg_t;

void log_test(uint32_t value)
{
    CAN_HandleTypeDef* hcan = &hcan2;

    test_msg_t message;
    message.header.IDE = CAN_ID_STD;
    message.header.RTR = CAN_RTR_DATA;
    message.header.DLC = sizeof(value);
    message.header.StdId = 0x100;

    (void) memcpy((void*) message.tx_data, (void*) &value, sizeof(value));
    // memset(message.tx_data, 0xFE, message.header.DLC);
    // message.tx_data[3] = 0x05;
    // message.tx_data[7] = 0x05;

    HAL_StatusTypeDef status = HAL_CAN_AddTxMessage(hcan,
                                                    &message.header,
                                                    message.tx_data,
                                                    &message.tx_mailbox);

    if (status != HAL_OK)
    {
        uint32_t err = HAL_CAN_GetError(hcan);
        __ASM("NOP");
    }

    while (HAL_CAN_IsTxMessagePending(hcan, message.tx_mailbox) != 0)
        ;

    __ASM("NOP");
}