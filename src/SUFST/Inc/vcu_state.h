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
typedef enum uint_16
{
    VCU_INIT = 0x00,      // VCU General Initialisation
    VCU_IDLE = 0x10,      // VCU Waiting for TS
    VCU_TS_ENABLE = 0x20, // VCU ENABLING TS
    VCU_TS_ON = 0x30,     // VCU TS IDLE
    VCU_R2D = 0x40,       // VCU R2D
    VCU_ERROR = 0xE0,     // VCU recoverable Error
    VCU_FATAL = 0xF0,     // VCU unrecoverable Error
} vcu_state_t;

#endif // VCU_STATE_H