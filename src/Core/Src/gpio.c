/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, ROB_LED_Pin|VC_LEDS_Pin|LED_4_Pin|R2D_LED_Pin
                          |LED_2_Pin|TS_ON_LED_Pin|TS_ON_Pin|BRAKELIGHT_EN_Pin
                          |STATUS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, RS232_FORCEOFF_Pin|RS232_FORCEON_Pin|IMU_MODE_Pin|EEPROM_WC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOF, R2D_SIREN_Pin|IMU_ADDR_Pin|LS_LED_6_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, SPARE_OUT_7_Pin|SPARE_OUT_6_Pin|SPARE_OUT_5_Pin|SPARE_OUT_4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, SPARE_OUT_3_Pin|DEBUG_GPIO_1_Pin|DEBUG_GPIO_2_Pin|DEBUG_GPIO_3_Pin
                          |DEBUG_GPIO_4_Pin|DEBUG_GPIO_5_Pin|DEBUG_GPIO_6_Pin|DEBUG_GPIO_7_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOG, DEBUG_GPIO_8_Pin|DEBUG_GPIO_9_Pin|DEBUG_GPIO_10_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : ROB_LED_Pin VC_LEDS_Pin LED_4_Pin R2D_LED_Pin
                           LED_2_Pin TS_ON_LED_Pin TS_ON_Pin BRAKELIGHT_EN_Pin
                           STATUS_Pin */
  GPIO_InitStruct.Pin = ROB_LED_Pin|VC_LEDS_Pin|LED_4_Pin|R2D_LED_Pin
                          |LED_2_Pin|TS_ON_LED_Pin|TS_ON_Pin|BRAKELIGHT_EN_Pin
                          |STATUS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : SD_CARD_DETECT_Pin */
  GPIO_InitStruct.Pin = SD_CARD_DETECT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(SD_CARD_DETECT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : RS232_BAD_Pin */
  GPIO_InitStruct.Pin = RS232_BAD_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(RS232_BAD_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : RS232_FORCEOFF_Pin RS232_FORCEON_Pin IMU_MODE_Pin EEPROM_WC_Pin */
  GPIO_InitStruct.Pin = RS232_FORCEOFF_Pin|RS232_FORCEON_Pin|IMU_MODE_Pin|EEPROM_WC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : TOO_HOT_Pin */
  GPIO_InitStruct.Pin = TOO_HOT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(TOO_HOT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : R2D_SIREN_Pin IMU_ADDR_Pin LS_LED_6_Pin */
  GPIO_InitStruct.Pin = R2D_SIREN_Pin|IMU_ADDR_Pin|LS_LED_6_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /*Configure GPIO pins : SPARE_OUT_7_Pin SPARE_OUT_6_Pin SPARE_OUT_5_Pin SPARE_OUT_4_Pin */
  GPIO_InitStruct.Pin = SPARE_OUT_7_Pin|SPARE_OUT_6_Pin|SPARE_OUT_5_Pin|SPARE_OUT_4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : WHEELSPEED_RR_Pin WHEELSPEED_RL_Pin WHEELSPEED_FR_Pin WHEELSPEED_FL_Pin */
  GPIO_InitStruct.Pin = WHEELSPEED_RR_Pin|WHEELSPEED_RL_Pin|WHEELSPEED_FR_Pin|WHEELSPEED_FL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : SPARE_IN_3_Pin GPS_PPS_Pin SPARE_IN_2_Pin */
  GPIO_InitStruct.Pin = SPARE_IN_3_Pin|GPS_PPS_Pin|SPARE_IN_2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : SPARE_OUT_3_Pin DEBUG_GPIO_1_Pin DEBUG_GPIO_2_Pin DEBUG_GPIO_3_Pin
                           DEBUG_GPIO_4_Pin DEBUG_GPIO_5_Pin DEBUG_GPIO_6_Pin DEBUG_GPIO_7_Pin */
  GPIO_InitStruct.Pin = SPARE_OUT_3_Pin|DEBUG_GPIO_1_Pin|DEBUG_GPIO_2_Pin|DEBUG_GPIO_3_Pin
                          |DEBUG_GPIO_4_Pin|DEBUG_GPIO_5_Pin|DEBUG_GPIO_6_Pin|DEBUG_GPIO_7_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : SPARE_OUT_2_Pin SPARE_OUT_1_Pin APPS_PWR_GOOD_Pin SPARE_IN_1_Pin
                           SW_BTN_2_Pin SW_BTN_1_Pin R2D_BTN_Pin */
  GPIO_InitStruct.Pin = SPARE_OUT_2_Pin|SPARE_OUT_1_Pin|APPS_PWR_GOOD_Pin|SPARE_IN_1_Pin
                          |SW_BTN_2_Pin|SW_BTN_1_Pin|R2D_BTN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pins : TS_ON_BTN_Pin TS_READY_Pin */
  GPIO_InitStruct.Pin = TS_ON_BTN_Pin|TS_READY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : DEBUG_GPIO_8_Pin DEBUG_GPIO_9_Pin DEBUG_GPIO_10_Pin */
  GPIO_InitStruct.Pin = DEBUG_GPIO_8_Pin|DEBUG_GPIO_9_Pin|DEBUG_GPIO_10_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  /*Configure GPIO pin : USER_BUTTON_Pin */
  GPIO_InitStruct.Pin = USER_BUTTON_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USER_BUTTON_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
