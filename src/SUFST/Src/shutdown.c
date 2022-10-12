/***************************************************************************
 * @file   shutdown.c
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-07-02
 * @brief  Shutdown check
 ***************************************************************************/

#include "shutdown.h"

#include "gpio.h"

/**
 * @brief   Check if shutdown fault has occurred
 *
 * @note    Shutdown is active low
 */
void check_shutdown()
{
    if (!HAL_GPIO_ReadPin(SHUTDOWN_IN_GPIO_Port, SHUTDOWN_IN_Pin))
    {
        // TODO: create fault handler
        Error_Handler();
    }
}