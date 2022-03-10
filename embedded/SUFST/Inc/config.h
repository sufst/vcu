/***************************************************************************
 * @file   config.h
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2022-02-08
 * @brief  System configuration
***************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

#include "tx_api.h"

/***************************************************************************
 * sensor thread
 ***************************************************************************/

#define THROTTLE_INPUT_RESOLUTION	16		// resolution of ADC
#define THROTTLE_SCALED_RESOLUTION	10		// scaled resolution sent to control thread
#define THROTTLE_DEADZONE_FRACTION	0.01	// fractional dead-zone below which throttle always zero
#define THROTTLE_MAX_DIFF_FRACTION	0.05	// fractional maximum allowed difference between ADC readings
#define THROTTLE_ENABLE_DIFF_CHECK	1		// set to 1 to enable check for discrepancy between ADC readings
#define THROTTLE_ENABLE_DEADZONE	0		// set to 1 to enable dead-zone

/***************************************************************************
 * testbenches
 ***************************************************************************/

// enable flags
#define RUN_THROTTLE_TESTBENCH		1		// throttle input from lookup table
#define RUN_SOME_OTHER_TESTBENCH	0		// placeholder

// testbench parameters
#define THROTTLE_TESTBENCH_LAPS 	4		// 1 for standing start only, 2+ to add flying laps

/***************************************************************************
 * safeguards
 ***************************************************************************/

// allow only one testbench to run at once
#if ((RUN_THROTTLE_TESTBENCH \
	  + RUN_SOME_OTHER_TESTBENCH) > 1)
	#error "Only one testbench can run at once"
#endif

#endif
