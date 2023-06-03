/******************************************************************************
 * @file    dash.h
 * @author  Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief   Interface with dash
 *****************************************************************************/

#ifndef DASH_H
#define DASH_H

#include <gpio.h>
#include <stdbool.h>
#include <stdint.h>

#include "config.h"

void dash_init(const config_vc_t* vc_config_ptr);

void dash_set_r2d_led_state(GPIO_PinState state);
void dash_set_ts_on_led_state(GPIO_PinState state);
void dash_set_drs_led_state(GPIO_PinState state);
void dash_toggle_ts_on_led();
bool dash_get_r2d_btn_state();
bool dash_get_ts_on_btn_state();
bool dash_get_drs_btn_state();

#endif