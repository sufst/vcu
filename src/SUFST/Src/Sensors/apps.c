/***************************************************************************
 * @file   apps.c
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @brief  Accelerator pedal position sensor
 ***************************************************************************/

#include "apps.h"

#include <adc.h>
#include <stdbool.h>

#include "config.h"
#include "scs.h"
#include "trace.h"

#define APPS_SCALED_MAX 1000 // maximum scaled value

/**
 * @brief Safety critical signal instances for APPS
 */
scs_t apps_signals[2];

#define NUM_PREV_INPUTS 50
static uint32_t last_inputs[NUM_PREV_INPUTS];

/*
 * function prototypes
 */
bool apps_inputs_agree(const uint32_t inputs[2]);

/**
 * @brief   Initialises APPS inputs
 */
void apps_init()
{
    scs_create(&apps_signals[0],
               &hadc1,
               APPS_1_ADC_MIN,
               APPS_1_ADC_MAX,
               0,
               APPS_SCALED_MAX);

    scs_create(&apps_signals[1],
               &hadc2,
               APPS_2_ADC_MIN,
               APPS_2_ADC_MAX,
               0,
               APPS_SCALED_MAX);
}

/**
 * @brief   Reads the APPS input
 *
 * @details The ADCs have a resolution of 16 bits which is then scaled down to
 *          10 bits
 */
uint32_t apps_read()
{
    const uint32_t num_inputs = 2;

    uint32_t inputs[num_inputs];
    unsigned long accumulator = 0;

    for (unsigned int i = 0; i < num_inputs; i++)
    {
        inputs[i] = scs_read(&apps_signals[i]);

#if !APPS_DISABLE_SCS_CHECK
        if (!scs_validate(&apps_signals[i]))
        {
            // TODO: create internal fault handler
            Error_Handler();
            inputs[i] = 0;
        }
#endif

        accumulator += inputs[i];
    }

#if !APPS_DISABLE_DIFF_CHECK
    if (!apps_inputs_agree(inputs))
    {
        // TODO: create internal error handler
        Error_Handler();
    }
#endif

    uint32_t new_input = accumulator / num_inputs;

    for (uint32_t i = 1; i < NUM_PREV_INPUTS; i++)
    {
        last_inputs[i - 1] = last_inputs[i];
    }
    last_inputs[NUM_PREV_INPUTS - 1] = new_input;

    // result
    uint32_t result = 0;
    for (uint32_t i = 0; i < NUM_PREV_INPUTS; i++)
    {
        result += last_inputs[i];
    }
    result /= NUM_PREV_INPUTS;

    return result;
}

/**
 * @brief       Checks for agreement between the two APPS input readings
 *
 * @param[in]   inputs  Array of two APPS input readings
 *
 * @return      true    The inputs agree
 * @return      false   The inputs differ significantly
 */
bool apps_inputs_agree(const uint32_t inputs[2])
{
    uint32_t diff = (inputs[1] > inputs[0]) ? inputs[1] - inputs[0]
                                            : inputs[0] - inputs[1];
    static const uint32_t max_diff = APPS_MAX_DIFF_FRACTION * APPS_SCALED_MAX;

    return diff < max_diff;
}