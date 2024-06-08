/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define VC_LEDS_Pin GPIO_PIN_3
#define VC_LEDS_GPIO_Port GPIOE
#define DRS_LED_Pin GPIO_PIN_4
#define DRS_LED_GPIO_Port GPIOE
#define TS_ON_LED_Pin GPIO_PIN_5
#define TS_ON_LED_GPIO_Port GPIOE
#define R2D_LED_Pin GPIO_PIN_6
#define R2D_LED_GPIO_Port GPIOE
#define USER_BUTTON_Pin GPIO_PIN_13
#define USER_BUTTON_GPIO_Port GPIOC
#define USER_BUTTON_EXTI_IRQn EXTI15_10_IRQn
#define TS_READY_Pin GPIO_PIN_1
#define TS_READY_GPIO_Port GPIOF
#define R2D_BTN_Pin GPIO_PIN_2
#define R2D_BTN_GPIO_Port GPIOF
#define SAGL_Pin GPIO_PIN_3
#define SAGL_GPIO_Port GPIOF
#define TS_ON_BTN_Pin GPIO_PIN_5
#define TS_ON_BTN_GPIO_Port GPIOF
#define DRS_BTN_Pin GPIO_PIN_10
#define DRS_BTN_GPIO_Port GPIOF
#define MCO_Pin GPIO_PIN_0
#define MCO_GPIO_Port GPIOH
#define BPS_ADC_Pin GPIO_PIN_0
#define BPS_ADC_GPIO_Port GPIOC
#define APPS_2_ADC_Pin GPIO_PIN_3
#define APPS_2_ADC_GPIO_Port GPIOC
#define DRS_Pin GPIO_PIN_0
#define DRS_GPIO_Port GPIOA
#define APPS_1_ADC_Pin GPIO_PIN_3
#define APPS_1_ADC_GPIO_Port GPIOA
#define GREEN_LED_Pin GPIO_PIN_0
#define GREEN_LED_GPIO_Port GPIOB
#define TS_ON_Pin GPIO_PIN_14
#define TS_ON_GPIO_Port GPIOF
#define R2D_SIREN_Pin GPIO_PIN_15
#define R2D_SIREN_GPIO_Port GPIOF
#define STATUS_Pin GPIO_PIN_9
#define STATUS_GPIO_Port GPIOE
#define BRAKE_PRESSED_Pin GPIO_PIN_11
#define BRAKE_PRESSED_GPIO_Port GPIOE
#define RED_LED_Pin GPIO_PIN_14
#define RED_LED_GPIO_Port GPIOB
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define BLUE_LED_Pin GPIO_PIN_7
#define BLUE_LED_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
