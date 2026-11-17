/******************************************************************************
 * @file    watchdog.h
 * @brief   Independent watchdog (IWDG): restarts the VCU if the control loop
 *          stops running
 *****************************************************************************/

#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <stdbool.h>

#include "status.h"

void watchdog_check_reset_cause(void);
bool watchdog_caused_last_reset(void);
status_t watchdog_start(void);
void watchdog_kick(void);

#endif
