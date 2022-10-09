/***************************************************************************
 * @file   rtcan.c
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief  RTOS wrapper around CAN bus
 ***************************************************************************/

#include "rtcan.h"

#include <stdbool.h>
#include <stdint.h>

#include <memory.h>

/**
 * internal functions
 */
static rtcan_status_t create_status(rtcan_context_t*);

/**
 * @brief       Initialises the RTCAN instance
 *
 * @details     The CAN instance of the handle should not be used by any other
 *              part of the system
 *
 * @param[in]   context_ptr     Pointer to RTCAN context
 * @param[in]   hcan            CAN handle
 */
rtcan_status_t rtcan_init(rtcan_context_t* context_ptr, CAN_HandleTypeDef* hcan)
{
    context_ptr->hcan = hcan;
    context_ptr->err = RTCAN_ERROR_NONE;

    // mailbox
    const uint32_t mailbox_size
        = sizeof(context_ptr->hcan->Instance->sTxMailBox)
          / sizeof(CAN_TxMailBox_TypeDef);

    ULONG tx_status
        = tx_semaphore_create(&context_ptr->sem, NULL, mailbox_size);

    if (tx_status != TX_SUCCESS)
    {
        context_ptr->err |= RTCAN_ERROR_INTERNAL;
    }

    // start peripheral
    HAL_StatusTypeDef hal_status = HAL_CAN_Start(context_ptr->hcan);

    if (hal_status != HAL_OK)
    {
        context_ptr->err |= RTCAN_ERROR_START;
    }

    return create_status(context_ptr);
}

/**
 * @brief
 *
 * @param[in]   context_ptr     Pointer to RTCAN context
 */
rtcan_status_t rtcan_transmit(rtcan_context_t* context_ptr,
                              uint32_t identifier,
                              uint8_t* data_ptr,
                              uint32_t data_length)
{
    if (data_ptr == NULL || data_length == 0)
    {
        context_ptr->err |= RTCAN_ERROR_ARG;
    }

    if (tx_semaphore_get(&context_ptr->sem, TX_WAIT_FOREVER) != TX_SUCCESS)
    {
        context_ptr->err |= RTCAN_ERROR_INTERNAL;
    }

    if (context_ptr->err == RTCAN_ERROR_NONE)
    {
        // create message
        CAN_TxHeaderTypeDef header = {
            .IDE = CAN_ID_STD,
            .RTR = CAN_RTR_DATA,
            .DLC = data_length,
            .StdId = identifier,
        };

        uint8_t tx_data[8];
        (void) memcpy(tx_data, data_ptr, data_length);

        // send it
        uint32_t tx_mailbox;

        HAL_StatusTypeDef hal_status = HAL_CAN_AddTxMessage(context_ptr->hcan,
                                                            &header,
                                                            tx_data,
                                                            &tx_mailbox);

        if (hal_status != HAL_OK)
        {
            context_ptr->err |= RTCAN_ERROR_INTERNAL;
        }
    }

    return create_status(context_ptr);
}

/**
 * @brief       Retrieves the error code
 *
 * @param[in]   context_ptr     Pointer to RTCAN context
 */
uint32_t rtcan_get_error(rtcan_context_t* context_ptr)
{
    return context_ptr->err;
}

/**
 * @brief       Create a status code based on the current error state
 *
 * @param[in]   context_ptr     Pointer to RTCAN context
 */
static rtcan_status_t create_status(rtcan_context_t* context_ptr)
{
    return (context_ptr->err == RTCAN_ERROR_NONE) ? RTCAN_OK : RTCAN_ERROR;
}