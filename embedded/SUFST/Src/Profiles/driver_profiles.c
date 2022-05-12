/***************************************************************************
 * @file   driver_profiles.h
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2022-02-13
 * @brief  Function implementation and lookup tables for driver profiles
 ***************************************************************************/

#include "driver_profiles.h"

/*
 * raw data for driver profiles defined in driver_profile_data.c
 */
extern uint16_t default_throttle_map [THROTTLE_MAP_SIZE];

/*
 * lookup table for all profiles
 */
static const driver_profile_t default_profile = {DRIVER_PROFILE_DEFAULT, "Default", default_throttle_map};

const driver_profile_t* driver_profiles [NUM_DRIVER_PROFILES] = {
	&default_profile,
};


/**
 * @brief 		Fetches a pointer to the requested driver profile
 *
 * @details		The driver profile is const and cannot be modified at runtime.
 *
 * @param[in]	profile_id		Unique ID number for driver profile
 * @param[out]	profile_ptr		Pointer to change to point to the driver profile
 *
 * @return		DRIVER_PROFILE_FOUND if the profile exists
 * 				DRIVER_PROFILE_INVALID if the profile does not exist
 */
UINT driver_profile_lookup(const driver_profile_t** profile_ptr, UINT profile_id)
{
	// check that the profile exists first
	if (profile_id >= NUM_DRIVER_PROFILES)
	{
		profile_ptr = NULL;
		return DRIVER_PROFILE_INVALID;
	}

	// the profile exists, get it from the look-up table
	*profile_ptr = driver_profiles[profile_id];
	return DRIVER_PROFILE_FOUND;
}