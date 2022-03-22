/***************************************************************************
 * @file   pm100.h
 * @author Chua Shen Yik (syc2e18@soton.ac.uk)
 * @date   2022-01-09
 * @brief  Inverter CAN communication prototypes
 ***************************************************************************/

#ifndef INC_PM100_H_
#define INC_PM100_H_

#include "can_types.h"
#include "tx_api.h"

void pm100_eeprom_write_blocking(uint16_t parameter_address, uint16_t data);
void pm100_eeprom_read_blocking(uint16_t parameter_address);
void pm100_command_tx(uint16_t torque_command, uint16_t speed_command, uint8_t direction_command, uint8_t inverter_enable, uint8_t inverter_discharge, uint8_t speed_mode_enable, uint16_t commanded_torque_limit);
void pm100_torque_command_tx(UINT torque_command);
void pm100_init();

#endif /* INC_PM100_H_ */
