#include "bps.h"

/**
 * @brief       Initialises BPS
 *
 * @param[in]   bps_ptr     BPS context
 * @param[in]   config_ptr  Configuration
 */
status_t bps_init(bps_context_t* bps_ptr,
                  log_context_t* log_ptr,
                  const config_bps_t* config_ptr)
{
    bps_ptr->config_ptr = config_ptr;
    bps_ptr->log_ptr = log_ptr;

    // compute pressure thresholds
    const uint16_t range
        = config_ptr->scs.max_mapped - config_ptr->scs.min_mapped;

    const uint16_t offset = config_ptr->scs.min_mapped;

    bps_ptr->fully_pressed_threshold
        = (config_ptr->fully_pressed_fraction * range) + offset;

    // create the SCS instance
    status_t status = scs_create(&bps_ptr->signal, &config_ptr->scs);

    return status;
}

/**
 * @brief       Reads the BPS
 *
 * @param[in]   bps_ptr
 * @param[out]  reading_ptr
 */
status_t bps_read(bps_context_t* bps_ptr, uint16_t* reading_ptr)
{
    status_t status = scs_read(&bps_ptr->signal, reading_ptr);
    scs_status_t status_verbose = bps_ptr->signal.status_verbose;

    if (status != STATUS_OK)
    {
        if (status_verbose == STATUS_THRESHOLD_ERROR)
        {
            LOG_INFO(bps_ptr->log_ptr, "BPS threshold error; ");
        }
        else
        {
            LOG_INFO(bps_ptr->log_ptr, "BPS unknown error; ");
        }
        status = STATUS_ERROR;
    }
    else if (status_verbose == STATUS_THRESHOLD_WARNING)
    {
        LOG_INFO(bps_ptr->log_ptr, "BPS threshold warning; ");
    }

    LOG_INFO(bps_ptr->log_ptr, "BPS reading: %d; ", *reading_ptr);

    return status;
}

/**
 * @brief
 *
 * @param[in]   bps_ptr     BPS context
 *
 * @retval  true    BPS is fully pressed
 * @retval  false   BPS not fully pressed, or SCS fault
 */
bool bps_fully_pressed(bps_context_t* bps_ptr)
{
    uint16_t reading = 0;
    status_t status = bps_read(bps_ptr, &reading);

    bool above_threshold = (reading > bps_ptr->fully_pressed_threshold);

    return (status == STATUS_OK) && above_threshold;
}

/**
 * @brief
 *
 * @param[in]   bps_ptr     BPS context
 *
 * @retval  true    BPS is pressed
 * @retval  false   BPS is not pressed, or SCS fault
 */
bool bps_valid(bps_context_t* bps_ptr)
{
    uint16_t reading = 0;
    status_t status = bps_read(bps_ptr, &reading);
    // if (status == STATUS_OK){
    //     if (reading != 0){
    //         return false;
    //     }
    // }
    // return true ;
    return (status == STATUS_OK) && (reading != 0);
}