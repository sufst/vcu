/***************************************************************************
 * @file   config_rules.h
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2022-03-15
 * @brief  Rules for checking system configuration against competition rules
 ***************************************************************************/

#include "config.h"
#include "tx_user.h"

/***************************************************************************
 * define rules / limits on parameters
 ***************************************************************************/

#define MAX_READY_TO_DRIVE_BUZZER_TIME 3
#define MIN_READY_TO_DRIVE_BUZZER_TIME 1

/***************************************************************************
 * work out any values needed to check the rules
 ***************************************************************************/

#define NUM_TESTBENCHES_RUNNING (RUN_APPS_TESTBENCH + RUN_FAULT_STATE_TESTBENCH)

#define TESTBENCHES_RUNNING     (NUM_TESTBENCHES_RUNNING > 0)

#define APPS_ADC_MAX            ((1 << APPS_ADC_RESOLUTION) - 1)

/***************************************************************************
 * general safeguards
 ***************************************************************************/

// APPS
#if APPS_ADC_RESOLUTION < APPS_SCALED_RESOLUTION
#error "Scaled APPS resolution is greater than input resolution"
#endif

#if APPS_1_ADC_MAX > APPS_ADC_MAX || APPS_2_ADC_MAX > APPS_ADC_MAX
#error "APPS ADC maximum greater than actual maximum ADC output"
#endif

#if APPS_1_ADC_MIN < 0 || APPS_1_ADC_MIN < 0
#error "APPS ADC minimum less than zero"
#endif

#if APPS_1_ADC_MIN >= APPS_1_ADC_MAX || APPS_2_ADC_MIN >= APPS_2_ADC_MAX
#error "APPS ADC minimum greater than or equal to APPS ADC maximum"
#endif

/***************************************************************************
 * competition rules
 ***************************************************************************/

#if COMPETITION_MODE

#ifndef TX_SAFETY_CRITICAL
#error "TX_SAFETY_CRITICAL not enabled"
#endif

// testbenches must be disabled
#if TESTBENCHES_RUNNING
#error "Testbenches are enabled"
#endif

// ready-to-drive cannot be bypassed
#if READY_TO_DRIVE_OVERRIDE
#error "Ready-to-drive overridden"
#endif

// inverter cannot be in speed mode
#if INVERTER_SPEED_MODE
#error "Inverter in speed mode"
#endif

// cannot disable APPS signal checks
#if APPS_DISABLE_DIFF_CHECK
#error "APPS input discrepancy check disabled"
#endif

// restrict ready-to-drive buzzer time
#if (READY_TO_DRIVE_BUZZER_TIME > MAX_READY_TO_DRIVE_BUZZER_TIME)
#error "Ready-to-drive buzzer will sound for too long"
#elif (READY_TO_DRIVE_BUZZER_TIME < MIN_READY_TO_DRIVE_BUZZER_TIME)
#error "Ready-to-drive buzzer will not sound for long enough"
#endif

#endif
