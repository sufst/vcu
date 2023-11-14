/***************************************************************************
 * @file   config.h
 * @author Ryuta Itabashi (@h2so4, ri1g22@soton.ac.uk)
 * @brief  Global configuration for on-car-telemetry
 ***************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

/**
 * @param CAN_DEBUG_MODE
 * 
 * 0 - Normal Mode - can_unpack thread consumes data from rtcan
 * 
 * 1 - Debug Mode - can_unpack thread gets simulated data from testbench
 *  
 */
#define CAN_DEBUG_MODE 0

#endif /* CONFIG_H */