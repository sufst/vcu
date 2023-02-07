/******************************************************************************
 * @file    shutdown.c
 * @author  Alexander Mills (Scaniox#7130 am9g22@soton.ac.uk)
 * @brief   shutdown interrupt causes the system to enter a shutdown thread
 *****************************************************************************/

#include "shutdown.h"

#include <stdint.h>

#include "tx_api.h"

#include "gpio.h"

#include <inttypes.h>

