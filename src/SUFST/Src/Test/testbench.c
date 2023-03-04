/***************************************************************************
 * @file   testbench.c
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2022-02-08
 * @brief  Testbench implementations
 ***************************************************************************/

#include "Test/testbench.h"

#include <stdbool.h>

#if RUN_FAULT_STATE_TESTBENCH
#include "fault.h"
#include "gpio.h"
#endif

#if RUN_APPS_TESTBENCH
#include "apps_testbench_data.h"
#endif

/***************************************************************************
 * APPS input testbench
 ***************************************************************************/

#if RUN_APPS_TESTBENCH

/**
 * @brief 	Testbench to simulate APPS inputs using data from a look-up
 *          table
 *
 * @details If APPS_TESTBENCH_LAPS (config.h) is set to 1, only the first
 *          lap with a standing start will be simulated. For values of 2 or
 *          more, additional flying laps are added. The simulated APPS input is
 *          the same for laps 2, 3, 4, etc.
 *
 * @return	Simulated APPS input for current system timestamp
 */
UINT testbench_apps_input()
{
    // persistent variables
    static UINT lap_count = 0;
    static UINT lookup_index = 0;
    static UINT last_lap_finish_time = 0;
    static UINT apps_input = 0;

    if (lap_count < APPS_TESTBENCH_LAPS)
    {
        // work out which lookup table to use
        // -> standing start data in first lap
        // -> flying lap data for any further laps
        const uint16_t* time_data;
        const uint16_t* apps_data;
        UINT final_lookup_index;

        {
            // first lap (standing start)
            if (lap_count == 0)
            {
                time_data = standing_start_time_lookup;
                apps_data = standing_start_apps_lookup;

                final_lookup_index = (sizeof(standing_start_time_lookup)
                                      / sizeof(standing_start_time_lookup[0]))
                                     - 1;
            }
            // flying laps
            else
            {
                time_data = flying_lap_time_lookup;
                apps_data = flying_lap_apps_lookup;

                final_lookup_index = (sizeof(flying_lap_time_lookup)
                                      / sizeof(flying_lap_time_lookup[0]))
                                     - 1;
            }
        }

        // timestamps in ms
        const UINT current_time
            = tx_time_get() * 1000 / TX_TIMER_TICKS_PER_SECOND;
        const UINT lap_finish_time = time_data[final_lookup_index];
        const UINT current_lap_time = current_time - last_lap_finish_time;

        // progress to next APPS input if in next time slot
        UINT next_time;
        bool end_of_lap = false;

        while (current_lap_time > (next_time = time_data[lookup_index + 1]))
        {
            lookup_index++;

            // check for end of lap
            if (next_time == lap_finish_time)
            {
                end_of_lap = true;
                break;
            }
        }

        apps_input = apps_data[lookup_index];

        // end of lap handling
        if (end_of_lap)
        {
            end_of_lap = false;
            lookup_index = 0;
            lap_count++;
            last_lap_finish_time = current_time;
        }
    }
    else
    {
        apps_input = 0;
    }

    return apps_input;
}

#endif

/***************************************************************************
 * fault state testbench
 ***************************************************************************/

#if RUN_FAULT_STATE_TESTBENCH

/**
 * @brief	Causes the system to enter the fault state if the 'USER' button
 * 			is pressed
 */
VOID testbench_fault_state()
{
    GPIO_PinState user_button_state
        = HAL_GPIO_ReadPin(USER_BUTTON_GPIO_Port, USER_BUTTON_Pin);

    if (user_button_state == GPIO_PIN_SET)
    {
        critical_fault(CRITICAL_FAULT_USER_BTN);
    }
}

#endif
