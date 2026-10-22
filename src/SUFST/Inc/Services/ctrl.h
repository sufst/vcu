/*****************************************************************************
 * @file    ctrl.h
 * @author  Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief   Control service
 * @details This service is responsible for controlling VCU based on driver
 *          inputs
 ****************************************************************************/

#ifndef CTRL_H
#define CTRL_H

#include <FreeRTOS.h>
#include <stdint.h>
#include <task.h>

#include "apps.h"
#include "bps.h"
#include "canbc.h"
#include "config.h"
#include "dash.h"
#include "fans.h"
#include "launch_ctrl.h"
#include "log.h"
#include "pm100.h"
#include "remote_ctrl.h"
#include "rtds.h"
#include "status.h"
#include "tick.h"
#include "torque_map.h"
#include "wheelspeed.h"

/*
 * error codes
 */
#define CTRL_ERROR_NONE 0x00
#define CTRL_ERROR_INIT 0x01               // service failed to initialise
#define CTRL_ERROR_TS_READY_TIMEOUT 0x02   // TS ready from TRC timed out
#define CTRL_ERROR_PRECHARGE_TIMEOUT 0x04  // precharge timed out
#define CTRL_ERROR_TRC_RUN_FAULT 0x08      // TRC faulted at runtime
#define CTRL_ERROR_INVERTER_RUN_FAULT 0x10 // inverter faulted at runtime
#define CTRL_ERROR_WATCHDOG_RESET 0x20     // the watchdog restarted the VCU (cleared by a fault ack)

/**
 * @brief   Control state
 */
typedef enum
{
    CTRL_STATE_TS_BUTTON_WAIT,
    CTRL_STATE_WAIT_NEG_AIR,
    CTRL_STATE_PRECHARGE_WAIT,
    CTRL_STATE_R2D_WAIT,
    CTRL_STATE_TS_ON,
    CTRL_STATE_R2D_OFF,
    CTRL_STATE_R2D_OFF_WAIT,
    CTRL_STATE_TS_ACTIVATION_FAILURE,
    CTRL_STATE_TS_RUN_FAULT,
    CTRL_STATE_SPIN,
    CTRL_STATE_APPS_SCS_FAULT,
    CTRL_STATE_APPS_BPS_FAULT,
} ctrl_state_t;

// Note if changing these, check the line in ctrl.c starting with
// `if (ctrl_ptr->current_mode == CTRL_MODE_UNKNOWN)`
// to allow R2D in any new modes
typedef enum
{
    CTRL_MODE_UNKNOWN = 0,
    CTRL_MODE_ENDURANCE = 1,
    CTRL_MODE_MAX = 2,
    CTRL_MODE_LAUNCH_CTRL = 3,
    CTRL_MODE_CRAWL = 4,
    CTRL_MODE_REVERSE = 5,
    CTRL_MODE_REMOTE_CTRL = 6,
    CTRL_MODE_UNKNOWN_7 = 7,
    CTRL_MODE_UNKNOWN_8 = 8,
    CTRL_MODE_UNKNOWN_9 = 9,
    CTRL_MODE_INVERTER_PROG = 10
} ctrl_mode_t;

/**
 * @brief   Control service context
 */
typedef struct
{
    ctrl_state_t state;          // state machine state
    ctrl_state_t prev_state;     // state on the previous tick
    bool rtds_active;            // ready-to-drive sound playing (R2D_WAIT -> TS_ON)
    uint8_t shdn_open_count;     // consecutive ticks the shutdown circuit read open
    TickType_t rtds_start;       // tick the ready-to-drive sound started
    ctrl_mode_t current_mode;    // current ctrl mode
    ctrl_mode_t requested_mode;  // requested ctrl mode
    TaskHandle_t thread;         // service thread
    uint16_t apps_reading;       // APPS reading (% * 10)
    uint16_t bps_reading;        // BPS reading (% * 10)
    uint16_t sagl_reading;       // steering angle reading (% * 40)
    uint16_t current_reading;    // current reading (% * 40)
    int16_t motor_speed_reading; // motor speed reading (rpm)
    float fl_wheel_rpm;       // front left wheel rpm
    float fr_wheel_rpm;       // front right wheel rpm
    uint32_t wheelspeed_sample_count; // wheelspeed sample counter (new front data when it changes)
    bool front_new_sample;    // a new wheelspeed sample arrived since the last control tick
    float electrical_power_w; // DC-bus power reading (W)
    uint16_t torque_request;  // last torque request
    uint8_t shdn_reading;
    int16_t motor_temp;
    int16_t inv_temp;
    int16_t max_temp;

    bool inverter_pwr;
    bool pump_pwr;

    TickType_t neg_air_start;
    TickType_t precharge_start; // precharge start time in ticks
    TickType_t motor_torque_zero_start;
    TickType_t apps_bps_start;
    TickType_t apps_bps_fault_start;
    dash_context_t *dash_ptr;   // dash service
    pm100_context_t *pm100_ptr; // PM100 service
    canbc_context_t *canbc_ptr; // CANBC service
    fans_context_t *fans_ptr;   // Fans service (reads sensor hub fan switch)
    tick_context_t *tick_ptr;   // tick thread (reads certain sensors)
    remote_ctrl_context_t *remote_ctrl_ptr; // tick thread (reads certain sensors)
    wheelspeed_context_t *wheelspeed_ptr; // wheelspeed service (front speed reference)
    torque_map_t torque_map;              // torque map (APPS -> torque request)
    launch_ctrl_context_t launch_ctrl;         // launch control (standing starts) + power limit
    const config_launch_ctrl_t *launch_ctrl_config_ptr; // launch control config
    TickType_t launch_ctrl_last_tick;          // tick of the previous launch control step
    TickType_t launch_ctrl_log_tick;           // rate limit for launch control fault log lines

    const config_ctrl_t *config_ptr;      // config
    const config_rtds_t *rtds_config_ptr; // RTDS config
    rtds_pulse_context_t rtds_pulse_ctx; // non-blocking RTDS reverse-pulse state

    uint8_t error; // error code

} ctrl_context_t;

/*
 * public functions
 */
status_t ctrl_init(ctrl_context_t *ctrl_ptr,
                   dash_context_t *dash_ptr,
                   pm100_context_t *pm100_ptr,
                   tick_context_t *tick_ptr,
                   remote_ctrl_context_t *remote_ctrl_ptr,
                   canbc_context_t *canbc_ptr,
                   fans_context_t *fans_ptr,
                   wheelspeed_context_t *wheelspeed_ptr,
                   const config_ctrl_t *config_ptr,
                   const config_rtds_t *rtds_config_ptr,
                   const config_torque_map_t *torque_map_config_ptr,
                   const config_launch_ctrl_t *launch_ctrl_config_ptr);

#endif
