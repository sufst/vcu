/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    app_threadx.c
  * @author  MCD Application Team
  * @brief   ThreadX applicative file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2020-2021 STMicroelectronics.
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
#include "control_thread.h"
#include "can_thread.h"
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
  (void)byte_pool;
  /* USER CODE END App_ThreadX_MEM_POOL */

  /* USER CODE BEGIN App_ThreadX_Init */


  VOID* stack_ptr;	// pointer to allocated memory for thread stack

  /**************************
   * Control thread creation
   **************************/

  // allocate memory for control thread from application memory pool
  ret = tx_byte_allocate(memory_ptr, &stack_ptr, CONTROL_THREAD_STACK_SIZE, TX_NO_WAIT);

  if (ret == TX_SUCCESS)
  {
	  // create control thread
	  ret = tx_thread_create(&control_thread, CONTROL_THREAD_NAME, control_thread_entry, 0, stack_ptr,
			  CONTROL_THREAD_STACK_SIZE, CONTROL_THREAD_PRIORITY, CONTROL_THREAD_PREEMPTION_THRESHOLD,
			  TX_NO_TIME_SLICE, TX_AUTO_START);
  }
  else
  {
	  // error, failed to allocate control thread stack from application memory pool
	  return ret;
  }

  /*************************
   * CAN thread creation
   **************************/

  // allocate memory for CAN thread from application memory pool
  if (ret == TX_SUCCESS)
  {
	  ret = tx_byte_allocate(memory_ptr, &stack_ptr, CAN_THREAD_STACK_SIZE, TX_NO_WAIT);
  }
  else
  {
	  // error, failed to create control thread
	  return ret;
  }

  if (ret == TX_SUCCESS)
  {
	  // create CAN thread
	  tx_thread_create(&can_thread, CAN_THREAD_NAME, can_thread_entry, 0, stack_ptr,
			  CAN_THREAD_STACK_SIZE, CAN_THREAD_PRIORITY, CAN_THREAD_PREEMPTION_THRESHOLD,
			  TX_NO_TIME_SLICE, TX_AUTO_START);
  }
  else
  {
	  // error, failed to allocate control thread stack from application memory pool
	  return ret;
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

/* USER CODE END 1 */
