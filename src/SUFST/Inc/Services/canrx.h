/******************************************************************************
 * @file    canrx.h
 * @author  Robert Kirkbride (@r-kirkbride, rgak1g24@soton.ac.uk)
 * @brief   Can Listener Service
 * @details This service is responsible for listening for CAN messages required
 *          by other services
 *****************************************************************************/

#ifndef CANRX_H
#define CANRX_H

#include <can_s.h>
#include <rtcan.h>
#include <stdint.h>
#include <tx_api.h>

#include "config.h"
#include "log.h"
#include "status.h"

#define CANRX_QUEUE_SIZE              10 // 10 items

#define CANRX_ERROR_NONE              0x00 // no errors
#define CANRX_ERROR_INIT              0x01 // initialisation error
#define CANRX_ERROR_BROADCAST_TIMEOUT 0x02 // no broadcasts received
#define CANRX_ERROR_POST_FAULT        0x04 // power-on self-test fault
#define CANRX_ERROR_RUN_FAULT         0x08 // runtime fault

/**
 * @brief   CANRX context
 */
typedef struct
{
    TX_THREAD thread;
    rtcan_handle_t* rtcan_c_ptr;
    rtcan_handle_t* rtcan_s_ptr;
    TX_QUEUE can_c_rx_queue;
    TX_QUEUE can_s_rx_queue;
    ULONG can_rx_queue_mem[CANRX_QUEUE_SIZE];
    TX_MUTEX state_mutex;
    bool broadcasts_valid;
    struct can_s_msgid_0_x201_t bms_pack;
    uint16_t error;
    const config_canrx_t* config_ptr;
} canrx_context_t;

/*
 * public functions
 */
status_t canrx_init(canrx_context_t* canrx_ptr,
                    rtcan_handle_t* rtcan_c_ptr,
                    rtcan_handle_t* rtcan_s_ptr,
                    TX_BYTE_POOL* stack_pool_ptr,
                    const config_canrx_t* config_ptr);

#endif
