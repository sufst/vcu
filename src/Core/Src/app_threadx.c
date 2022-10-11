/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_threadx.c
  * @author  MCD Application Team
  * @brief   ThreadX applicative file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
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
#include "app_threadx.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "init.h"
#include "rtcan.h"
#include "canbc.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/**
 * @brief RTCAN service for sensor (S) CAN bus
 */
static rtcan_handle_t rtcan_s;
static canbc_handle_t can_broadcaster;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/**
  * @brief  Application ThreadX Initialization.
  * @param memory_ptr: memory pointer
  * @retval int
  */
UINT App_ThreadX_Init(VOID *memory_ptr)
{
  UINT ret = TX_SUCCESS;
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;

  /* USER CODE BEGIN App_ThreadX_MEM_POOL */
  /* USER CODE END App_ThreadX_MEM_POOL */

  /* USER CODE BEGIN App_ThreadX_Init */
  init_threads(byte_pool);

  // TODO: move this somewhere else?
  // TODO: check error codes
  rtcan_init(&rtcan_s, &hcan2, 4, byte_pool);
  rtcan_start(&rtcan_s);

  canbc_init(&can_broadcaster, &rtcan_s, 4, byte_pool);
  canbc_start(&can_broadcaster);
  /* USER CODE END App_ThreadX_Init */

  return ret;
}

/**
  * @brief  MX_ThreadX_Init
  * @param  None
  * @retval None
  */
void MX_ThreadX_Init(void)
{
  /* USER CODE BEGIN  Before_Kernel_Start */

  /* USER CODE END  Before_Kernel_Start */

  tx_kernel_enter();

  /* USER CODE BEGIN  Kernel_Start_Error */

  /* USER CODE END  Kernel_Start_Error */
}

/* USER CODE BEGIN 1 */

/*
 * these callbacks post to the RTCAN service semaphores to allow a new message
 * to be transmitted on the corresponding bus
 */
void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef* can_h)
{
    (void) rtcan_tx_mailbox_callback(&rtcan_s, can_h);
}

void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef* can_h)
{
    (void) rtcan_tx_mailbox_callback(&rtcan_s, can_h);
}

void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef* can_h)
{
    (void) rtcan_tx_mailbox_callback(&rtcan_s, can_h);
}
/* USER CODE END 1 */
