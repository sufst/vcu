#include "bps.h"

/**
 * @brief       Initialises BPS
 *
 * @param[in]   bps_ptr     BPS context
 * @param[in]   config_ptr  Configuration
 */
status_t bps_init(bps_context_t *bps_ptr, const config_bps_t *config_ptr)
{
    bps_ptr->config_ptr = config_ptr;

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
status_t bps_read(bps_context_t *bps_ptr, uint16_t *reading_ptr)
{
    status_t status = scs_read(&bps_ptr->signal, reading_ptr);
    scs_status_t status_verbose = bps_ptr->signal.status_verbose;

    if (status != STATUS_OK)
    {
        if (status_verbose == STATUS_THRESHOLD_ERROR)
        {
            LOG_INFO("BPS threshold error;\n");
        }
        else
        {
            LOG_INFO("BPS unknown error;\n");
        }
        status = STATUS_ERROR;
    }
    else if (status_verbose == STATUS_THRESHOLD_WARNING)
    {
        LOG_DEBUG("BPS threshold warning;\n");
    }

    LOG_DEBUG("BPS reading: %d;\n", *reading_ptr);

    return status;
}
