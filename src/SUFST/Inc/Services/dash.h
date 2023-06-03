/*****************************************************************************
 * @file    dash.h
 * @author  Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief   Dash service
 * @details This service is responsible for interfacing with the dash, reading
 *          inputs and controlling LEDs. The priority of this service should
 *          generally be lower than those controlling the tractive system.
 ****************************************************************************/

#ifndef DASH_H
#define DASH_H

#include <gpio.h>
#include <stdbool.h>
#include <stdint.h>
#include <tx_api.h>

#include "config.h"

/**
 * @brief   Dash service context
 */
typedef struct
{
    TX_THREAD thread;                 // dash service thread
    const config_vc_t* vc_config_ptr; // visual check configuration

} dash_context_t;

/*
 * public functions
 */
void dash_init(dash_context_t* dash_ptr,
               TX_BYTE_POOL* stack_pool_ptr,
               const config_thread_t* thread_config_ptr,
               const config_vc_t* vc_config_ptr);

void dash_set_r2d_led_state(GPIO_PinState state);
void dash_set_ts_on_led_state(GPIO_PinState state);
void dash_set_drs_led_state(GPIO_PinState state);
void dash_toggle_ts_on_led();
bool dash_get_r2d_btn_state();
bool dash_get_ts_on_btn_state();
bool dash_get_drs_btn_state();

#endif