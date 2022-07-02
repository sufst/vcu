/***************************************************************************
 * @file   bps.h
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-07-02
 * @brief  BPS input implementation
 ***************************************************************************/

#include "bps.h"

#include "config.h"
#include "fault.h"

#include "scs.h"

/**
 * @brief Safety critical signal instance for BPS
 */
static scs_t bps_signal;

/**
 * @brief Initialise BPS
 */
void bps_init()
{
    uint32_t bps_signal_max = (1 << BPS_SCALED_RESOLUTION) - 1;

    scs_create(&bps_signal,
               &hadc3,
               BPS_ADC_MIN,
               BPS_ADC_MAX,
               0,
               bps_signal_max);
}

/**
 * @brief Read BPS
 */
uint32_t bps_read()
{
    uint32_t reading = scs_read(&bps_signal);

    if (!scs_validate(&bps_signal))
    {
        critical_fault(CRITICAL_FAULT_SCS_OUTSIDE_BOUNDS);
    }

    return reading;
}