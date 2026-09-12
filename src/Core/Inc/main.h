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
#define ROB_LED_Pin GPIO_PIN_2
#define ROB_LED_GPIO_Port GPIOE
#define SD_CARD_DETECT_Pin GPIO_PIN_3
#define SD_CARD_DETECT_GPIO_Port GPIOE
#define RS232_BAD_Pin GPIO_PIN_6
#define RS232_BAD_GPIO_Port GPIOE
#define RS232_FORCEOFF_Pin GPIO_PIN_13
#define RS232_FORCEOFF_GPIO_Port GPIOC
#define RS232_FORCEON_Pin GPIO_PIN_14
#define RS232_FORCEON_GPIO_Port GPIOC
#define TOO_HOT_Pin GPIO_PIN_2
#define TOO_HOT_GPIO_Port GPIOF
#define R2D_SIREN_Pin GPIO_PIN_6
#define R2D_SIREN_GPIO_Port GPIOF
#define PWM_SPARE_2_Pin GPIO_PIN_7
#define PWM_SPARE_2_GPIO_Port GPIOF
#define IMU_ADDR_Pin GPIO_PIN_10
#define IMU_ADDR_GPIO_Port GPIOF
#define IMU_MODE_Pin GPIO_PIN_0
#define IMU_MODE_GPIO_Port GPIOC
#define EEPROM_WC_Pin GPIO_PIN_1
#define EEPROM_WC_GPIO_Port GPIOC
#define SPARE_ADC_Pin GPIO_PIN_3
#define SPARE_ADC_GPIO_Port GPIOA
#define DASH_ROTARY_ADC_Pin GPIO_PIN_4
#define DASH_ROTARY_ADC_GPIO_Port GPIOA
#define CT_ADC_Pin GPIO_PIN_5
#define CT_ADC_GPIO_Port GPIOA
#define SAGL_ADC_Pin GPIO_PIN_6
#define SAGL_ADC_GPIO_Port GPIOA
#define BPS_R_ADC_Pin GPIO_PIN_7
#define BPS_R_ADC_GPIO_Port GPIOA
#define BPS_F_ADC_Pin GPIO_PIN_4
#define BPS_F_ADC_GPIO_Port GPIOC
#define APPS_2_ADC_Pin GPIO_PIN_5
#define APPS_2_ADC_GPIO_Port GPIOC
#define APPS_1_ADC_Pin GPIO_PIN_0
#define APPS_1_ADC_GPIO_Port GPIOB
#define PWM_SPARE_1_Pin GPIO_PIN_1
#define PWM_SPARE_1_GPIO_Port GPIOB
#define LS_LED_6_Pin GPIO_PIN_13
#define LS_LED_6_GPIO_Port GPIOF
#define VC_LEDS_Pin GPIO_PIN_7
#define VC_LEDS_GPIO_Port GPIOE
#define LED_4_Pin GPIO_PIN_8
#define LED_4_GPIO_Port GPIOE
#define R2D_LED_Pin GPIO_PIN_9
#define R2D_LED_GPIO_Port GPIOE
#define LED_2_Pin GPIO_PIN_10
#define LED_2_GPIO_Port GPIOE
#define TS_ON_LED_Pin GPIO_PIN_11
#define TS_ON_LED_GPIO_Port GPIOE
#define TS_ON_Pin GPIO_PIN_13
#define TS_ON_GPIO_Port GPIOE
#define BRAKELIGHT_EN_Pin GPIO_PIN_14
#define BRAKELIGHT_EN_GPIO_Port GPIOE
#define STATUS_Pin GPIO_PIN_15
#define STATUS_GPIO_Port GPIOE
#define SPARE_OUT_7_Pin GPIO_PIN_10
#define SPARE_OUT_7_GPIO_Port GPIOB
#define FAN_PWM_R_Pin GPIO_PIN_11
#define FAN_PWM_R_GPIO_Port GPIOB
#define SPARE_OUT_6_Pin GPIO_PIN_12
#define SPARE_OUT_6_GPIO_Port GPIOB
#define SPARE_OUT_5_Pin GPIO_PIN_13
#define SPARE_OUT_5_GPIO_Port GPIOB
#define SPARE_OUT_4_Pin GPIO_PIN_14
#define SPARE_OUT_4_GPIO_Port GPIOB
#define FAN_PWM_L_Pin GPIO_PIN_15
#define FAN_PWM_L_GPIO_Port GPIOB
#define WHEELSPEED_RR_Pin GPIO_PIN_8
#define WHEELSPEED_RR_GPIO_Port GPIOD
#define WHEELSPEED_RR_EXTI_IRQn EXTI9_5_IRQn
#define WHEELSPEED_RL_Pin GPIO_PIN_9
#define WHEELSPEED_RL_GPIO_Port GPIOD
#define WHEELSPEED_RL_EXTI_IRQn EXTI9_5_IRQn
#define WHEELSPEED_FR_Pin GPIO_PIN_10
#define WHEELSPEED_FR_GPIO_Port GPIOD
#define WHEELSPEED_FR_EXTI_IRQn EXTI15_10_IRQn
#define WHEELSPEED_FL_Pin GPIO_PIN_11
#define WHEELSPEED_FL_GPIO_Port GPIOD
#define WHEELSPEED_FL_EXTI_IRQn EXTI15_10_IRQn
#define SPARE_IN_3_Pin GPIO_PIN_12
#define SPARE_IN_3_GPIO_Port GPIOD
#define GPS_PPS_Pin GPIO_PIN_13
#define GPS_PPS_GPIO_Port GPIOD
#define SPARE_IN_2_Pin GPIO_PIN_14
#define SPARE_IN_2_GPIO_Port GPIOD
#define SPARE_OUT_3_Pin GPIO_PIN_15
#define SPARE_OUT_3_GPIO_Port GPIOD
#define SPARE_OUT_2_Pin GPIO_PIN_2
#define SPARE_OUT_2_GPIO_Port GPIOG
#define SPARE_OUT_1_Pin GPIO_PIN_3
#define SPARE_OUT_1_GPIO_Port GPIOG
#define APPS_PWR_GOOD_Pin GPIO_PIN_4
#define APPS_PWR_GOOD_GPIO_Port GPIOG
#define SPARE_IN_1_Pin GPIO_PIN_5
#define SPARE_IN_1_GPIO_Port GPIOG
#define SW_BTN_2_Pin GPIO_PIN_6
#define SW_BTN_2_GPIO_Port GPIOG
#define SW_BTN_1_Pin GPIO_PIN_7
#define SW_BTN_1_GPIO_Port GPIOG
#define R2D_BTN_Pin GPIO_PIN_8
#define R2D_BTN_GPIO_Port GPIOG
#define TS_ON_BTN_Pin GPIO_PIN_6
#define TS_ON_BTN_GPIO_Port GPIOC
#define TS_READY_Pin GPIO_PIN_7
#define TS_READY_GPIO_Port GPIOC
#define DEBUG_GPIO_1_Pin GPIO_PIN_0
#define DEBUG_GPIO_1_GPIO_Port GPIOD
#define DEBUG_GPIO_2_Pin GPIO_PIN_1
#define DEBUG_GPIO_2_GPIO_Port GPIOD
#define DEBUG_GPIO_3_Pin GPIO_PIN_3
#define DEBUG_GPIO_3_GPIO_Port GPIOD
#define DEBUG_GPIO_4_Pin GPIO_PIN_4
#define DEBUG_GPIO_4_GPIO_Port GPIOD
#define DEBUG_GPIO_5_Pin GPIO_PIN_5
#define DEBUG_GPIO_5_GPIO_Port GPIOD
#define DEBUG_GPIO_6_Pin GPIO_PIN_6
#define DEBUG_GPIO_6_GPIO_Port GPIOD
#define DEBUG_GPIO_7_Pin GPIO_PIN_7
#define DEBUG_GPIO_7_GPIO_Port GPIOD
#define DEBUG_GPIO_8_Pin GPIO_PIN_9
#define DEBUG_GPIO_8_GPIO_Port GPIOG
#define DEBUG_GPIO_9_Pin GPIO_PIN_10
#define DEBUG_GPIO_9_GPIO_Port GPIOG
#define DEBUG_GPIO_10_Pin GPIO_PIN_11
#define DEBUG_GPIO_10_GPIO_Port GPIOG
#define CAN_T_RX_Pin GPIO_PIN_5
#define CAN_T_RX_GPIO_Port GPIOB
#define CAN_T_TX_Pin GPIO_PIN_6
#define CAN_T_TX_GPIO_Port GPIOB
#define USER_BUTTON_Pin GPIO_PIN_7
#define USER_BUTTON_GPIO_Port GPIOB
#define CAN_S_RX_Pin GPIO_PIN_8
#define CAN_S_RX_GPIO_Port GPIOB
#define CAN_S_TX_Pin GPIO_PIN_9
#define CAN_S_TX_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
