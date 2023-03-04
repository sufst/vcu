/******************************************************************************
 * @file    shutdown.c
 * @author  Alexander Mills (Scaniox#7130 am9g22@soton.ac.uk)
 * @brief   shutdown interrupt causes the system to enter a shutdown thread
 *****************************************************************************/

#include "shutdown.h"

#include <inttypes.h>
#include <stdint.h>

#include "gpio.h"
#include "tx_api.h"

// // ISR for all EXTI pin change interupts
// void HAL_GPIO_EXTI_Callback(uint16_t GPIO_pin)
// {
//     if (GPIO_pin == SHUTDOWN_IN_Pin)
//     {
//         __ASM("NOP");
//     }

//     // start shutdown thread
// }