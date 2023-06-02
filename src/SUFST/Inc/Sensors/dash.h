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

void dash_init(bool vc_enable,
               uint32_t vc_ticks,
               bool vc_all_leds,
               uint32_t vc_stagger_ticks);
void dash_set_r2d_led_state(GPIO_PinState state);
void dash_set_ts_on_led_state(GPIO_PinState state);
void dash_set_drs_led_state(GPIO_PinState state);

#endif