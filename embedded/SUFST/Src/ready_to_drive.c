/***************************************************************************
 * @file   ready_to_drive.c
 * @author Tim Brewis (@t-bre, tab1g19@soton.ac.uk)
 * @date   2022-03-10
 * @brief  Ready to drive logic implementation
 ***************************************************************************/

#include "ready_to_drive.h"
#include "config.h"
#include "tx_api.h"

#include "gpio.h"
#include "pm100.h"

#include <stdbool.h>

/*
 * function prototypes
 */
static bool rtd_input_active();
static void sound_buzzer();

/**
 * @brief Wait for ready-to-drive signal to become active
 */
void rtd_wait()
{
	HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_SET);

	// wait for active high
	while (!rtd_input_active());

	// if ready to drive overridden ('USER' button input)
	// wait for button to be released
#if (READY_TO_DRIVE_OVERRIDE)
	while (rtd_input_active());
#endif

	pm100_disable();
	HAL_GPIO_WritePin(RTD_OUT_GPIO_Port, RTD_OUT_Pin, GPIO_PIN_RESET); // active low
	sound_buzzer();
	HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);
}

/**
 * @brief  	Check ready-to-drive signal
 *
 * @details If READY_TO_DRIVE_OVERRIDE is set, the 'USER' push button acts as
 * 			the ready-to-drive signal. Otherwise the actual signal is used.
 *
 * @retval 	true:	ready-to-drive signal active
 * 			false:	ready-to-drive signal inactive otherwise
 */
bool rtd_input_active()
{
#if (READY_TO_DRIVE_OVERRIDE)
	return HAL_GPIO_ReadPin(USER_BUTTON_GPIO_Port, USER_BUTTON_Pin) == GPIO_PIN_SET;
#else
	return HAL_GPIO_ReadPin(READY_TO_DRIVE_Port, READY_TO_DRIVE_Pin) == GPIO_PIN_SET;
#endif
}

/**
 * @brief Output a high signal to the pin connected to the ready-to-drive buzzer
 */
void sound_buzzer()
{
	HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
	tx_thread_sleep(READY_TO_DRIVE_BUZZER_TIME);
	HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
}

