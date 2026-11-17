/***************************************************************************
 * @file   io.h
 * @author Robert Kirkbride (@r-kirkbride, rgak1g24@soton.ac.uk)
 * @author Adam Eastman (@Agwld, ace1g24@soton.ac.uk)
 * @brief  I/O interface
 ***************************************************************************/

#ifndef IO_H
#define IO_H

#include "gpio.h"
#include <stdbool.h>
#include <stdint.h>

#include "status.h"

/* Input and output functions
   STAG 12 VCU (PCB revision v3.0.0 onwards) uses TXH0137D-Q1 and TPL7407L logic level inverters on
   digital I/O lines. Always use the board-abstraction helpers below rather than
   calling the HAL functions directly to ensure intended behavior.
*/

// e.g. VCU_Output_High() sets the output high, which due to the inverter will result in a low voltage on the pin.

status_t VCU_Output_High(GPIO_TypeDef *port, uint16_t pin);
status_t VCU_Output_Low(GPIO_TypeDef *port, uint16_t pin);
status_t VCU_Output_Write(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state);
status_t VCU_Output_Toggle(GPIO_TypeDef *port, uint16_t pin);
bool VCU_Input_Read(GPIO_TypeDef *port, uint16_t pin);


#endif // IO_H