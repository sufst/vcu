/***************************************************************************
 * @file   testbench.h
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2022-02-08
 * @brief  Testbench function prototypes
***************************************************************************/

#ifndef TESTBENCH_H
#define TESTBENCH_H

#include "../config.h"
#include "tx_api.h"

/***************************************************************************
 * throttle input testbench
 ***************************************************************************/

#if RUN_THROTTLE_TESTBENCH

UINT testbench_throttle();

#endif

/***************************************************************************
 * fault state testbench
 ***************************************************************************/

#if RUN_FAULT_STATE_TESTBENCH

VOID testbench_fault_state();

#endif

#endif
