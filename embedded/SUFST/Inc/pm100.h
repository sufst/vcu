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

/**
  * @brief  pm100 Status structures definition
  */
typedef enum
{
  PM100_OK       = 0x00,
  PM100_ERROR    = 0x01,
  PM100_BUSY     = 0x02,
  PM100_TIMEOUT  = 0x03
} pm100_status_t;

/**
 * @brief	 in command message
 *
 */
typedef struct
{
	uint16_t	torque_command;
	uint16_t	speed_command;
	uint8_t		direction;
	uint8_t		inverter_enable;
	uint8_t		inverter_discharge;
	uint8_t		speed_mode_enable;
	uint16_t	commanded_torque_limit;
} pm100_command_t;

pm100_status_t pm100_eeprom_write_blocking(uint16_t parameter_address, uint16_t data);
pm100_status_t pm100_eeprom_read_blocking(uint16_t parameter_address);
pm100_status_t pm100_command_tx(pm100_command_t* command_data);
pm100_status_t pm100_torque_command_tx(UINT torque_command);
#if RUN_SPEED_MODE
pm100_status_t pm100_speed_command_tx(UINT speed);
#endif
pm100_status_t pm100_init();

#endif /* INC_PM100_H_ */
