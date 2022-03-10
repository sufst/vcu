/***************************************************************************
 * @file   ready_to_drive.c
 * @author Tim Brewis (tab1g19@soton.ac.uk)
 * @date   2022-03-10
 * @brief  Ready to drive logic implementation
 ***************************************************************************/

#include "ready_to_drive.h"
#include "config.h"

#include "gpio.h"
#include "stdbool.h"
#include "tx_api.h"

/*
 * function prototypes
 */
bool ready_to_drive_state();

/**
 * @brief Wait for ready-to-drive signal to become active
 */
void wait_for_ready_to_drive()
{
	HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_SET);
	while (!ready_to_drive_state());
	HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);
}

/**
 * @brief  	Check ready-to-drive signal
 *
 * @details If READY_TO_DRIVE_OVERRIDE is set, the 'USER' push button acts as
 * 			the ready-to-drive signal. Otherwise the actual signal is used.
 *
 * @return 	True if ready-to-drive signal active, false otherwise
 */
bool ready_to_drive_state()
{
#if (READY_TO_DRIVE_OVERRIDE)
	return HAL_GPIO_ReadPin(USER_BUTTON_GPIO_Port, USER_BUTTON_Pin) == GPIO_PIN_SET;
#else
	return HAL_GPIO_ReadPin(READY_TO_DRIVE_Port, READY_TO_DRIVE_Pin) == GPIO_PIN_SET;
#endif
}
