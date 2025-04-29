/******************************************************************************
 * @file    vcu_state.h
 * @author  Daniel hobbs (@D4nDude, dakh1g20@soton.ac.uk)
 * @brief   VCU State Machine Definition
 *****************************************************************************/

#ifndef VCU_STATE_H
#define VCU_STATE_H

#define VCU_STATE_MAJOR            0xF0

#define VCU_STATE_GET_MAJOR(state) (state | VCU_STATE_MAJOR)

/**
 * @brief       VCU state machine
 *
 * @details     Contains all states the VCU could be in.
 */
typedef enum
{
    VCU_STATE_INIT = 0x00,         // * VCU General Initialisation
    VCU_STATE_INIT_RTCAN,          // VCU RTCAN service init
    VCU_STATE_INIT_CANBC,          // VCU CANBC service init
    VCU_STATE_INIT_DASH,           // VCU Dash service init
    VCU_STATE_INIT_TICK,           // VCU tick service init
    VCU_STATE_INIT_PM100,          // PM100 service init
    VCU_STATE_INIT_CONTROL,        // Ctrl service init
    VCU_STATE_INIT_REMOTE_CONTROL, // Remote Control service init
    VCU_STATE_INIT_HEARTBEAT,      // Hearbeat service init
    VCU_STATE_INIT_DONE = 0x0F,    // Init Done

    VCU_STATE_IDLE = 0x10,     // * VCU Waiting for TS
    VCU_STATE_TS_START = 0x20, // * VCU ENABLING TS
    VCU_STATE_TS_ON = 0x30,    // * VCU TS IDLE
    VCU_STATE_R2D = 0x40,      // * VCU R2D
    VCU_STATE_ERROR = 0xE0,    // * VCU recoverable Error
    VCU_STATE_FATAL = 0xF0,    // * VCU unrecoverable Error
} vcu_state_t;

void vcu_set_state(const vcu_state_t newState);

vcu_state_t vcu_get_state();

#endif // VCU_STATE_H