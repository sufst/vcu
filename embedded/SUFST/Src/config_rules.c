/***************************************************************************
 * @file   config_rules.h
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2022-03-15
 * @brief  Rules for checking system configuration against competition rules
 ***************************************************************************/

#include "config.h"

/***************************************************************************
 * define rules / limits on parameters
 ***************************************************************************/

#define MAX_READY_TO_DRIVE_BUZZER_TIME 3
#define MIN_READY_TO_DRIVE_BUZZER_TIME 1

/***************************************************************************
 * work out any values needed to check the rules
 ***************************************************************************/

#define NUM_TESTBENCHES_RUNNING \
    (RUN_THROTTLE_TESTBENCH + RUN_FAULT_STATE_TESTBENCH)

#define TESTBENCHES_RUNNING (NUM_TESTBENCHES_RUNNING > 0)

/***************************************************************************
 * general safeguards
 ***************************************************************************/

#if APPS_ADC_RESOLUTION < APPS_SCALED_RESOLUTION
#error "Scaled APPS resolution is greater than input resolution"
#endif

/***************************************************************************
 * competition rules
 ***************************************************************************/

#if (COMPETITION_MODE)

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

// restrict ready-to-drive buzzer time
#if (READY_TO_DRIVE_BUZZER_TIME > MAX_READY_TO_DRIVE_BUZZER_TIME)
#error "Ready-to-drive buzzer will sound for too long"
#elif (READY_TO_DRIVE_BUZZER_TIME < MIN_READY_TO_DRIVE_BUZZER_TIME)
#error "Ready-to-drive buzzer will not sound for long enough"
#endif

#endif
