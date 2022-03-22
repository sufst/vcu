/***************************************************************************
 * @file   pm100.h
 * @author Chua Shen Yik (syc2e18@soton.ac.uk)
 * @date   2022-01-09
 * @brief  Inverter CAN communication prototypes
 ***************************************************************************/

#ifndef INC_PM100_H_
#define INC_PM100_H_

#include "can_types.h"

void parameter_write_command(uint16_t parameter_address, uint16_t data);
void parameter_read_command(uint16_t parameter_address);
void command_msg(uint16_t torque_command, uint16_t speed_command, uint8_t direction_command, uint8_t inverter_enable, uint8_t inverter_discharge, uint8_t speed_mode_enable, uint16_t commanded_torque_limit);
void pm100_torque_command(UINT torque_command);
void pm100_init();

#endif /* INC_PM100_H_ */
