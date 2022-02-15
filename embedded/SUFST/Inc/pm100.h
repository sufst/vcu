#ifndef INC_PM100_H_
#define INC_PM100_H_

#include "can_types.h"

void pm100_init();

void parameter_write_command(uint16_t parameter_address, uint16_t data);
void parameter_read_command(uint16_t parameter_address);
void command_msg(uint16_t torque_command, uint16_t speed_command, uint8_t direction_command, uint8_t inverter_enable, uint8_t inverter_discharge, uint8_t speed_mode_enable, uint16_t commanded_torque_limit);
void pm100_torque_command(uint16_t torque_command);



#endif /* INC_PM100_H_ */
