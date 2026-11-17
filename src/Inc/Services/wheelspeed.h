/******************************************************************************
 * @file    wheelspeed.h
 * @author  Martin Perreau (@maartin0)
 * @brief   Wheelspeed interrupt reader for 4 wheels
 * @details Computes wheelspeeds on the fly for 4 wheelspeed sensors,
            needs to handle ~4000 interrupts/second
            and broadcasts results in both RPM and m/s over CAN-S
 *****************************************************************************/

#ifndef WHEELSPEED_H
#define WHEELSPEED_H

#include <FreeRTOS.h>
#include <rtcan.h>
#include <semphr.h>
#include <stdbool.h>
#include <stdint.h>
#include <task.h>

#include "config.h"
#include "status.h"

typedef struct
{
    volatile uint32_t isr_count;     // live count updated by ISR
    volatile uint32_t last_edge;     // DWT cycle count of the latest edge, set by ISR
    uint32_t ref_count;              // pulse count at the reference edge
    uint32_t ref_edge;               // cycle count at the reference edge
    bool ref_valid;                  // false until an edge has been seen after standstill
    float rpm;                       // last accepted reading
    float rpm_out;                   // reading after the drop filter (what is reported)
    uint8_t drop_samples;            // consecutive samples a sudden drop has been held for
} wheel_state_t;

typedef struct
{
    float fl_mps;
    float fr_mps;
    float rl_mps;
    float rr_mps;
    float fl_rpm;
    float fr_rpm;
    float rl_rpm;
    float rr_rpm;
    uint32_t sample_count; // increments on every new sample (detects new data)
} wheelspeed_reading_t;

typedef struct
{
    TaskHandle_t thread;
    SemaphoreHandle_t reading_mutex;
    StaticSemaphore_t reading_mutex_cb;
    rtcan_handle_t *rtcan_h;
    wheel_state_t wheel_fr;
    wheel_state_t wheel_fl;
    wheel_state_t wheel_rr;
    wheel_state_t wheel_rl;
    wheelspeed_reading_t last_reading;
    const config_wheelspeed_t *config_ptr;
    TickType_t tx_error_log_tick; // rate limits the transmit failure log
} wheelspeed_context_t;

status_t wheelspeed_init(wheelspeed_context_t *wheelspeed_h,
                         rtcan_handle_t *rtcan_h,
                         const config_wheelspeed_t *config_ptr);

void wheelspeed_handle_exti(wheelspeed_context_t *wheelspeed_h, uint16_t gpio_pin);

status_t wheelspeed_get_speeds(wheelspeed_context_t *wheelspeed_h,
                               wheelspeed_reading_t *result);

#endif
