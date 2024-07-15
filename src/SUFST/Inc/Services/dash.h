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
#include "status.h"

typedef struct
{
    bool active_state;       // state considered active
    bool require_release;    // check for active then not active
    bool last_state;         // state memory
    uint32_t active_start;   // start tick of being active
    uint32_t required_ticks; // number of ticks for which must be active
    uint32_t sample_ticks;   // sleep ticks between samples
    GPIO_TypeDef* port;      // input GPIO port
    uint16_t pin;            // input GPIO pin
} input_context_t;


/**
 * @brief   Dash service context
 */
typedef struct
{
     TX_THREAD thread;                // dash service thread
     TX_TIMER ts_on_toggle_timer;     // timer for toggling TS on LED
     const config_dash_t* config_ptr; // configuration

     input_context_t r2d, tson;
     uint8_t r2d_flag, tson_flag;
} dash_context_t;

/*
 * public functions
 */
status_t dash_init(dash_context_t* dash_ptr,
                   TX_BYTE_POOL* stack_pool_ptr,
                   const config_dash_t* config_ptr);

status_t dash_blink_ts_on_led(dash_context_t* dash_ptr, uint32_t ticks);
status_t dash_set_ts_on_led_state(dash_context_t* dash_ptr,
                                  GPIO_PinState state);
status_t dash_wait_for_ts_on(dash_context_t* dash_ptr);
status_t dash_wait_for_r2d(dash_context_t* dash_ptr);
status_t dash_set_r2d_led_state(dash_context_t* dash_ptr, GPIO_PinState state);

void dash_update_buttons(dash_context_t *dash_ptr);
void dash_clear_buttons(dash_context_t *dash_ptr);

// void dash_set_r2d_led_state(GPIO_PinState state);
// void dash_set_drs_led_state(GPIO_PinState state);
// bool dash_get_r2d_btn_state();
// bool dash_get_ts_on_btn_state();
// bool dash_get_drs_btn_state();

#endif
