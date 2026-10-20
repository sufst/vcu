#include "rs232.h"

#include "main.h"

void rs232_enable(void)
{
    // pull the TRS3221E FORCEON low and FORCEOFF high: normal operation mode
    HAL_GPIO_WritePin(RS232_FORCEON_GPIO_Port, RS232_FORCEON_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RS232_FORCEOFF_GPIO_Port, RS232_FORCEOFF_Pin, GPIO_PIN_SET);
}