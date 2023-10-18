/*****************************************************************************
 * @file   thermistor_spoofer.h
 * @author Toby Godfrey (tmag1g21@soton.ac.uk)
 * @brief  PoC for thermistor messages for a custom TEM
 ****************************************************************************/

#ifndef THERMISTOR_SPOOFER_H
#define THERMISTOR_SPOOFER_H

#include <stdint.h>

#include "tx_api.h"

#include "canbc.h"

/**
 * @brief   Thermistor spoofer handle
 */
typedef struct
{
    /**
     * @brief   Input processing thread
     */
    TX_THREAD thread;

    /**
     * @brief   Sampling tick timer
     *
     * @details This event causes the thread to wake which will result in
     *          a whole bunch of other processing to act on the driver input
     */
    TX_TIMER tick_timer;

    /**
     * @brief   State mutex for CAN broadcast
     */
    TX_MUTEX state_mutex;

    /**
     * @brief   Current error code
     */
    uint32_t err;

} therm_spoof_handle_t;

/**
 * @brief   Spoofer status
 */
typedef enum
{
    THERM_SPOOF_OK,
    THERM_SPOOF_ERROR
} therm_spoof_status_t;

/*
 * function prototypes
 */
therm_spoof_status_t driver_ctrl_init(driver_ctrl_handle_t* driver_ctrl_h,
                                      ts_ctrl_handle_t* ts_ctrl_h,
                                      canbc_handle_t* canbc_h,
                                      TX_BYTE_POOL* stack_pool_ptr);

therm_spoof_status_t driver_ctrl_start(driver_ctrl_handle_t* driver_ctrl_h);

#endif
