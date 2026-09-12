#include "rs232.h"

#include "main.h"

void rs232_enable(void)
{
    // pull the TRS3221E FORCEON low and FORCEOFF high: normal operation mode
    HAL_GPIO_WritePin(RS232_FORCEON_GPIO_Port, RS232_FORCEON_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RS232_FORCEOFF_GPIO_Port, RS232_FORCEOFF_Pin, GPIO_PIN_SET);
}

void rs232_disable(void)
{
    // pull the TRS3221E FORCEOFF low
    HAL_GPIO_WritePin(RS232_FORCEOFF_GPIO_Port, RS232_FORCEOFF_Pin, GPIO_PIN_RESET);
}

bool rs232_is_valid(void)
{
    return HAL_GPIO_ReadPin(RS232_BAD_GPIO_Port, RS232_BAD_Pin) == GPIO_PIN_SET;
}
