/******************************************************************************
 * @file    watchdog.c
 * @brief   Independent watchdog (IWDG): restarts the VCU if the control loop
 *          stops running
 *
 * @details The IWDG runs from the internal ~32 kHz LSI oscillator, so it keeps
 *          counting even if the CPU, its clock or the software is stuck. Once
 *          started it can't be stopped (only a reset stops it), and it must be
 *          kicked before it counts down to zero, otherwise it resets the MCU.
 *
 *          - Timeout: reload 500 with prescaler 32 (see iwdg.c) = 500 ms at
 *            32 kHz. The LSI is only accurate to about 17..47 kHz, so the real
 *            timeout is somewhere between ~340 ms and ~940 ms.
 *          - It is started by the control task on its first cycle and kicked
 *            on every cycle (10 ms), so it resets the VCU if the control loop
 *            stops (task deadlock, stuck in a loop, starved, CPU lockup, ...).
 *            It is not started if initialisation fails (the car then can't be
 *            driven and the heartbeat LED blinks fast) or in USB mode.
 *          - Error_Handler() keeps kicking it, so a detected fault stays in
 *            its safe state (TS off) with the LED blinking, instead of
 *            rebooting.
 *          - It is frozen while the debugger halts the CPU.
 *          - After it has reset the VCU, the next boot logs a warning and sets
 *            CTRL_ERROR_WATCHDOG_RESET in the VCU error CAN frame.
 *****************************************************************************/

#include "watchdog.h"

#include "iwdg.h"
#include "main.h"

static bool reset_by_watchdog = false;
static bool started = false;

/**
 * @brief   Records whether the last reset was caused by the watchdog, then
 *          clears the reset flags (call once, at the start of main())
 */
void watchdog_check_reset_cause(void)
{
    reset_by_watchdog = (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != 0U);
    __HAL_RCC_CLEAR_RESET_FLAGS();
}

/**
 * @brief   Returns true if the watchdog caused the last reset
 */
bool watchdog_caused_last_reset(void)
{
    return reset_by_watchdog;
}

/**
 * @brief   Starts the watchdog (from then on it must be kicked regularly)
 */
status_t watchdog_start(void)
{
    if (!started)
    {
        // don't let the watchdog reset the VCU while stopped at a breakpoint
        __HAL_DBGMCU_FREEZE_IWDG();

        MX_IWDG_Init();
        started = true;
    }

    return STATUS_OK;
}

/**
 * @brief   Reloads the watchdog counter
 */
void watchdog_kick(void)
{
    if (started)
    {
        (void)HAL_IWDG_Refresh(&hiwdg);
    }
}
