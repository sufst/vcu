/***************************************************************************
 * @file   driver_profiles.h
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2022-02-13
 * @brief  Function prototypes and definitions for driver profiles
 ***************************************************************************/

#ifndef DRIVER_PROFILES_H
#define DRIVER_PROFILES_H

#include <stdint.h>
#include "tx_api.h"

#define THROTTLE_MAP_SIZE	1024

/*
 * driver profile IDs
 */
#define DRIVER_PROFILE_DEFAULT	0	// a default linear profile with a small deadzone
#define NUM_DRIVER_PROFILES		1	// 1 profile exists, any IDs less than this number are valid

/*
 * return codes
 */
#define DRIVER_PROFILE_FOUND	0	// the driver profile exists
#define DRIVER_PROFILE_INVALID	1	// the driver profile does not exist

/**
 * @brief Structure to store a driver profile
 */
typedef struct {
	UINT id;
	const CHAR* name;
	uint16_t* throttle_curve;
} driver_profile_t;

/*
 * function prototypes
 */
UINT driver_profile_lookup(const driver_profile_t** profile_ptr, UINT profile_id);

#endif
