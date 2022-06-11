/***************************************************************************
 * @file   drs.h
 * @author Chua Shen Yik (@hashyaha, syc2e18@soton.ac.uk)
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-06-08
 * @brief  DRS CAN interface prototypes
 ***************************************************************************/

#ifndef DRS_H
#define DRS_H

#include <stdint.h>
#include "dac.h"

#define DRS_DAC_HANDLE      hdac1

/* TODO: Determine the CAN ID from Dash */
#define DRS_CAN_ID 0xFF

/**
 * @brief Return codes
 */
typedef enum drs_status_e
{
    DRS_OK = 0x00,
    DRS_ERROR = 0x01,
    DAC_ERROR = 0x02,
    DRS_DAC_ERROR = 0x03,
} drs_status_t;

/*
 * function prototypes
 */
drs_status_t drs_init();

drs_status_t drs_read_state(uint32_t* value_ptr);
void drs_update_state(uint32_t, uint32_t value);

drs_status_t set_dac_val();

#endif
