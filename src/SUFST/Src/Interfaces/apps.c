#include "apps.h"

/**
 * @brief       Initialises the APPS
 *
 * @param[in]   apps_ptr    APPS context
 */
status_t apps_init(apps_context_t* apps_ptr, const config_apps_t* config_ptr)
{
    apps_ptr->config_ptr = config_ptr;
    apps_ptr->scs_error = SCS_ERROR_NONE;

    // initialise both SCS instances
    status_t status
        = scs_create(&apps_ptr->apps_1_signal, &config_ptr->apps_1_scs);

    if (status == STATUS_OK)
    {
        status = scs_create(&apps_ptr->apps_2_signal, &config_ptr->apps_2_scs);
    }

    return status;
}
/**
 * @brief       Reads the APPS
 *
 * @details     If either of the APPS signals is invalid, or there is a
 *              discrepancy between them, the output is set to zero
 *
 * @param[in]   apps_ptr        APPS instance
 * @param[out]  reading_ptr     Output reading
 *
 * @retval  STATUS_OK       Neither SCS has faulted and both APPS signals agree
 * @retval  STATUS_ERROR    SCS fault, or signals disagree
 */
status_t apps_read(apps_context_t* apps_ptr, uint16_t* reading_ptr)
{
    status_t status = STATUS_OK;
    uint16_t reading_1 = 0;
    uint16_t reading_2 = 0;

    // read both signals
    status_t status_1 = scs_read(&apps_ptr->apps_1_signal, &reading_1);
    status_t status_2 = scs_read(&apps_ptr->apps_2_signal, &reading_2);

    if (status_1 != STATUS_OK)
    {
        status = STATUS_ERROR;
        apps_ptr->scs_error |= SCS_ERROR_APPS1;
    }

    if (status_2 != STATUS_OK)
    {
        status = STATUS_ERROR;
        apps_ptr->scs_error |= SCS_ERROR_APPS2;
    }

    // check for discrepancy
    uint16_t diff = (reading_1 > reading_2) ? (reading_1 - reading_2)
                                            : (reading_2 - reading_1);

    if (diff > apps_ptr->config_ptr->max_discrepancy)
    {
        status = STATUS_ERROR;
        apps_ptr->scs_error |= SCS_ERROR_APPS_DISCREPANCY;
    }

    // return reading
    if (status == STATUS_OK)
    {
        *reading_ptr = (reading_1 + reading_2) / 2;
    }
    else
    {
        *reading_ptr = 0;
    }

    return status;
}

// #define APPS_SCALED_MAX 1000 // maximum scaled value

// /**
//  * @brief Safety critical signal instances for APPS
//  */
// scs_t apps_signals[2];

// #define NUM_PREV_INPUTS 50
// static uint32_t last_inputs[NUM_PREV_INPUTS];

// /*
//  * function prototypes
//  */
// bool apps_inputs_agree(const uint32_t inputs[2]);

// /**
//  * @brief   Initialises APPS inputs
//  */
// void apps_init()
// {
//     scs_create(&apps_signals[0],
//                &hadc1,
//                APPS_1_ADC_MIN,
//                APPS_1_ADC_MAX,
//                0,
//                APPS_SCALED_MAX);

//     scs_create(&apps_signals[1],
//                &hadc2,
//                APPS_2_ADC_MIN,
//                APPS_2_ADC_MAX,
//                0,
//                APPS_SCALED_MAX);
// }

// /**
//  * @brief   Reads the APPS input
//  *
//  * @details The ADCs have a resolution of 16 bits which is then scaled down
//  to
//  *          10 bits
//  */
// uint32_t apps_read()
// {
//     const uint32_t num_inputs = 2;

//     uint32_t inputs[num_inputs];
//     unsigned long accumulator = 0;

//     for (unsigned int i = 0; i < num_inputs; i++)
//     {
//         inputs[i] = scs_read(&apps_signals[i]);

// #if !APPS_DISABLE_SCS_CHECK
//         if (!scs_validate(&apps_signals[i]))
//         {
//             // TODO: create internal fault handler
//             Error_Handler();
//             inputs[i] = 0;
//         }
// #endif

//         accumulator += inputs[i];
//     }

// #if !APPS_DISABLE_DIFF_CHECK
//     if (!apps_inputs_agree(inputs))
//     {
//         // TODO: create internal error handler
//         Error_Handler();
//     }
// #endif

//     uint32_t new_input = accumulator / num_inputs;

//     for (uint32_t i = 1; i < NUM_PREV_INPUTS; i++)
//     {
//         last_inputs[i - 1] = last_inputs[i];
//     }
//     last_inputs[NUM_PREV_INPUTS - 1] = new_input;

//     // result
//     uint32_t result = 0;
//     for (uint32_t i = 0; i < NUM_PREV_INPUTS; i++)
//     {
//         result += last_inputs[i];
//     }
//     result /= NUM_PREV_INPUTS;

//     return result;
// }

// /**
//  * @brief       Checks for agreement between the two APPS input readings
//  *
//  * @param[in]   inputs  Array of two APPS input readings
//  *
//  * @return      true    The inputs agree
//  * @return      false   The inputs differ significantly
//  */
// bool apps_inputs_agree(const uint32_t inputs[2])
// {
//     uint32_t diff = (inputs[1] > inputs[0]) ? inputs[1] - inputs[0]
//                                             : inputs[0] - inputs[1];
//     static const uint32_t max_diff = APPS_MAX_DIFF_FRACTION *
//     APPS_SCALED_MAX;

//     return diff < max_diff;
// }