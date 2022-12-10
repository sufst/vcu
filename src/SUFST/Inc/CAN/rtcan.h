/***************************************************************************
 * @file   rtcan.h
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief  RTOS wrapper around CAN bus
 ***************************************************************************/

#ifndef RTCAN_H
#define RTCAN_H

#include "tx_api.h"

#include "rtcan.h"

#include "can.h"

/*
 * error codes
 */
#define RTCAN_ERROR_NONE 0x00000000U // no error
#define RTCAN_ERROR_INIT 0x00000001U // failed to start service
#define RTCAN_ERROR_ARG  0x00000002U // invalid argument
#define RTCAN_ERROR_QUEUE_FULL \
    0x00000004U // too many messages queued for transmit
#define RTCAN_ERROR_INTERNAL 0x80000000U // internal error

/**
 * @brief   RTCAN status
 */
typedef enum
{
    RTCAN_OK,
    RTCAN_ERROR
} rtcan_status_t;

/**
 * @brief   RTCAN message
 *
 * @note    This must have a size that is a multiple of sizeof(ULONG) for use
 *          with TX_QUEUE
 */
typedef struct
{
    /**
     * @brief   CAN standard identifier for message
     */
    uint32_t identifier;

    /**
     * @brief   Message data buffer
     */
    uint8_t data[8];

    /**
     * @brief   Length of message data in bytes
     */
    uint32_t length;

} rtcan_msg_t;

/*
 * queue sizing constants
 */
#define RTCAN_TX_QUEUE_LENGTH    100
#define RTCAN_TX_QUEUE_ITEM_SIZE (sizeof(rtcan_msg_t) / sizeof(ULONG))
#define RTCAN_TX_QUEUE_SIZE      (RTCAN_TX_QUEUE_LENGTH * RTCAN_TX_QUEUE_ITEM_SIZE)

/**
 * @brief RTCAN handle
 */
typedef struct
{
    /**
     * @brief   Service thread
     */
    TX_THREAD thread;

    /**
     * @brief   CAN handle dedicated to this instance
     */
    CAN_HandleTypeDef* hcan;

    /**
     * @brief   Transmit message box semaphore
     *
     * @details This holds a count of the number of currently available
     *          transmit message boxes and should be posted to when a message
     *          box becomes available again (e.g. in interrupt). See docs for
     *          rtcan_handle_tx_mailbox_callback().
     */
    TX_SEMAPHORE tx_mailbox_sem;

    /**
     * @brief   Transmit queue
     */
    TX_QUEUE tx_queue;

    /**
     * @brief   Transmit queue memory area
     */
    ULONG tx_queue_mem[RTCAN_TX_QUEUE_SIZE];

    /**
     * @brief   Current error code
     */
    uint32_t err;

} rtcan_handle_t;

/*
 * function prototypes
 */
rtcan_status_t rtcan_init(rtcan_handle_t* rtcan_h,
                          CAN_HandleTypeDef* hcan,
                          ULONG priority,
                          TX_BYTE_POOL* stack_pool_ptr);

rtcan_status_t rtcan_start(rtcan_handle_t* rtcan_h);

rtcan_status_t rtcan_transmit(rtcan_handle_t* rtcan_h, rtcan_msg_t* msg_ptr);

rtcan_status_t rtcan_handle_tx_mailbox_callback(rtcan_handle_t* rtcan_h,
                                                const CAN_HandleTypeDef* can_h);

uint32_t rtcan_get_error(rtcan_handle_t* rtcan_h);

#endif