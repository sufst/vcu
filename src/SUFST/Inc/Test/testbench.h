/***************************************************************************
 * @file   testbench.h
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2022-02-08
 * @brief  Testbench function prototypes
 ***************************************************************************/

#ifndef TESTBENCH_H
#define TESTBENCH_H

#include "tx_api.h"

#include "../config.h"

/***************************************************************************
 * APPS input testbench
 ***************************************************************************/

#if RUN_APPS_TESTBENCH

UINT testbench_apps_input();

#endif

/***************************************************************************
 * fault state testbench
 ***************************************************************************/

#if RUN_FAULT_STATE_TESTBENCH

VOID testbench_fault_state();

#endif

#endif
