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
 * error cores
 */
#define RTCAN_ERROR_NONE     0x00000000U // no error
#define RTCAN_ERROR_INIT     0x00000001U // failed to start service
#define RTCAN_ERROR_ARG      0x00000002U // invalid argument
#define RTCAN_ERROR_INTERNAL 0x80000000U // internal error

/**
 * @brief RTCAN context
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
     *          box becomes available again (e.g. in interrupt)
     */
    TX_SEMAPHORE sem;

    /**
     * @brief   Current error code
     */
    uint32_t err;

} rtcan_context_t;

/**
 * @brief   RTCAN status
 */
typedef enum
{
    RTCAN_OK,
    RTCAN_ERROR
} rtcan_status_t;

/*
 * function prototypes
 */
rtcan_status_t
rtcan_init(rtcan_context_t*, CAN_HandleTypeDef*, ULONG, TX_BYTE_POOL*);

rtcan_status_t rtcan_start(rtcan_context_t*);

rtcan_status_t
rtcan_transmit(rtcan_context_t*, uint32_t, const uint8_t*, uint32_t);

uint32_t rtcan_get_error(rtcan_context_t*);

#endif