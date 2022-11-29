/******************************************************************************
 * @file    shutdown.c
 * @author  Alexander Mills (Scaniox#7130 am9g22@soton.ac.uk)
 * @brief   shutdown interrupt causes the system to enter a shutdown thread
 *****************************************************************************/

#include <stdint.h>
#include <inttypes.h>
#include "tx_api.h"
#include "gpio.h"
#include "shutdown.h"

// ISR for all EXTI pin change interupts
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_pin)
{
    if(GPIO_pin == SHUTDOWN_IN_Pin)
    {
        uint8_t x = 1;
        while(1);
    }

    // start shutdown thread
}