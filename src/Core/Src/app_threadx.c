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
#include "vcu.h"
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
 * @brief   VCU instance
 */
static vcu_handle_t vcu;

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

  // initialise system
  if (ret == TX_SUCCESS)
  {
    vcu_status_t status = vcu_init(&vcu, &hcan2, byte_pool);

    if (status != VCU_OK)
    {
        ret = TX_START_ERROR;
    }
  }

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
    (void) vcu_handle_can_tx_mailbox_callback(&vcu, can_h);
}

void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef* can_h)
{
    (void) vcu_handle_can_tx_mailbox_callback(&vcu, can_h);
}

void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef* can_h)
{
    (void) vcu_handle_can_tx_mailbox_callback(&vcu, can_h);
}
/* USER CODE END 1 */
