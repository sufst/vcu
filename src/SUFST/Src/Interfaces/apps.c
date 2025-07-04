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
    status_t status = scs_create(&apps_ptr->apps_1_signal, &config_ptr->apps_1_scs);

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
status_t apps_read(apps_context_t *apps_ptr, uint16_t *reading_ptr)
{
    status_t status = STATUS_OK;
    uint16_t reading_1 = 0;
    uint16_t reading_2 = 0;

    /* static uint64_t count = 0; */

    /* uint8_t max_torque_percentage = 10; */
    /* uint8_t time_s = 30; */

    /* *reading_ptr */
    /*     = (uint16_t) ((1000 * (max_torque_percentage / 100)) / (time_s *
     * 100)) */
    /*       * count; // At 100Hz */
    /*                // 1000 is the max mapped value for APPS (config.c) */

    /* count = (count >= (time_s * 100)) ? count : count + 1; */

    // read both signals
    status_t status_1 = scs_read(&apps_ptr->apps_1_signal, &reading_1);
    scs_status_t status_1_verbose = apps_ptr->apps_1_signal.status_verbose;
    status_t status_2 = scs_read(&apps_ptr->apps_2_signal, &reading_2);
    scs_status_t status_2_verbose = apps_ptr->apps_2_signal.status_verbose;
    uint16_t adc_reading = get_adc(&apps_ptr->apps_2_signal);

    if (status_1 != STATUS_OK)
    {
        if (status_1_verbose == STATUS_THRESHOLD_ERROR)
        {
            LOG_INFO("APPS1 threshold error; ");
        }
        else
        {
            LOG_INFO("APPS1 unknown error; ");
        }
        status = STATUS_ERROR;
        apps_ptr->scs_error |= SCS_ERROR_APPS1;
    }
    else if (status_1_verbose == STATUS_THRESHOLD_WARNING)
    {
        LOG_INFO("APPS1 threshold warning; ");
    }

    // LOG_INFO("APPS1 reading: %d; ", reading_1);

    if (status_2 != STATUS_OK)
    {
        if (status_2_verbose == STATUS_THRESHOLD_ERROR)
        {
            LOG_INFO("APPS2 threshold error; ");
        }
        else
        {
            LOG_INFO("APPS2 unknown error; ");
        }
        status = STATUS_ERROR;
        apps_ptr->scs_error |= SCS_ERROR_APPS2;
    }
    else if (status_2_verbose == STATUS_THRESHOLD_WARNING)
    {
        LOG_INFO("APPS2 threshold warning; ");
    }

    // LOG_INFO("APPS2 reading: %d; ", reading_2);

    // // check for discrepancy
    uint16_t diff = (reading_1 > reading_2) ? (reading_1 - reading_2)
                                            : (reading_2 - reading_1);

    if (diff > apps_ptr->config_ptr->max_discrepancy)
    {
        LOG_INFO("DIFF error; ");
        status = STATUS_ERROR;
        apps_ptr->scs_error |= SCS_ERROR_APPS_DISCREPANCY;
    }

    // return reading
    if (status == STATUS_OK)
    {
        *reading_ptr = reading_2; //(reading_1 + reading_2) / 2;
    }
    else
    {
        *reading_ptr = 0;
    }

    LOG_INFO("\n");

    return status;
}

/**
 * @brief       Reads the APPS to check if it is plausible
 *
 * @details     This should be used when you don't care what the actual reading
 *              is, but you want to know if the APPS is plausible
 *
 * @param[in]   apps_ptr    APPS context
 */
bool apps_check_plausibility(apps_context_t *apps_ptr)
{
    uint16_t reading = 0;
    status_t status = apps_read(apps_ptr, &reading);
    return (status == STATUS_OK);
}
