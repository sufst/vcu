/***************************************************************************
 * @file   fault.h
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-05-21
 * @brief  Fault handling definitions and prototypes
 ***************************************************************************/

#ifndef FAULT_H
#define FAULT_H

/**
 * @brief Minor fault
 */
typedef enum minor_fault_e {
    MINOR_FAULT_CAN_TX_DROPPED,
    MINOR_FAULT_CAN_RX_DROPPED,
} minor_fault_t;

/**
 * @brief Critical fault
 */
typedef enum critical_fault_e {
    CRITICAL_FAULT_INIT_ERROR,
    CRITICAL_FAULT_THROTTLE_INPUT_DISCREPANCY,
    CRITICAL_FAULT_QUEUE_FULL,
    CRITICAL_FAULT_DRIVER_PROFILE_NOT_FOUND,
    CRITICAL_FAULT_USER_BTN,
} critical_fault_t;

/*
 * function prototypes
 */
void minor_fault(minor_fault_t fault);
void critical_fault(critical_fault_t fault);

#endif