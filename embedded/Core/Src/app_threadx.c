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

#include "config.h"
#include "trace.h"

#include "can_tx_thread.h"
#include "can_rx_thread.h"
#include "control_thread.h"
#include "fault_state_thread.h"
#include "sensor_thread.h"
#include "messaging_system.h"

#include "pm100.h"
#include "rtc_time.h"
#include "ready_to_drive.h"

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

    ret = tx_byte_allocate(memory_ptr, &stack_ptr, CONTROL_THREAD_STACK_SIZE, TX_NO_WAIT);

    if (ret == TX_SUCCESS)
    {
        ret = tx_thread_create(&control_thread, CONTROL_THREAD_NAME, control_thread_entry, 0, stack_ptr,
                    CONTROL_THREAD_STACK_SIZE, CONTROL_THREAD_PRIORITY, CONTROL_THREAD_PREEMPTION_THRESHOLD,
                    TX_NO_TIME_SLICE, TX_AUTO_START);
    }

    /*************************
     * CAN thread creation
     **************************/

    if (ret == TX_SUCCESS)
    {
        ret = tx_byte_allocate(memory_ptr, &stack_ptr, CAN_TX_THREAD_STACK_SIZE, TX_NO_WAIT);
    }

    if (ret == TX_SUCCESS)
    {
        ret = tx_thread_create(&can_tx_thread, CAN_TX_THREAD_NAME, can_tx_thread_entry, 0, stack_ptr,
                    CAN_TX_THREAD_STACK_SIZE, CAN_THREAD_PRIORITY, CAN_TX_THREAD_PREEMPTION_THRESHOLD,
                    TX_NO_TIME_SLICE, TX_AUTO_START);
    }

    if (ret == TX_SUCCESS)
    {
        ret = tx_byte_allocate(memory_ptr, &stack_ptr, CAN_RX_THREAD_STACK_SIZE, TX_NO_WAIT);
    }

    if (ret == TX_SUCCESS)
    {
        ret = tx_thread_create(&can_rx_thread, CAN_RX_THREAD_NAME, can_rx_thread_entry, 0, stack_ptr,
                    CAN_RX_THREAD_STACK_SIZE, CAN_RX_THREAD_PRIORITY, CAN_RX_THREAD_PREEMPTION_THRESHOLD,
                    TX_NO_TIME_SLICE, TX_AUTO_START);
    }

    /*************************
     * Sensor thread creation
     **************************/

    if (ret == TX_SUCCESS)
    {
        ret = tx_byte_allocate(memory_ptr, &stack_ptr, SENSOR_THREAD_STACK_SIZE, TX_NO_WAIT);

    }

    if (ret == TX_SUCCESS)
    {
        ret = tx_thread_create(&sensor_thread, SENSOR_THREAD_NAME, sensor_thread_entry, 0, stack_ptr,
                    SENSOR_THREAD_STACK_SIZE, SENSOR_THREAD_PRIORITY, SENSOR_THREAD_PREEMPTION_THRESHOLD,
                    TX_NO_TIME_SLICE, TX_AUTO_START);
    }

    /*************************
     * Fault thread creation
     **************************/

    // allocate memory for fault state thread from application memory pool
    if (ret == TX_SUCCESS)
    {
        ret = tx_byte_allocate(memory_ptr, &stack_ptr, FAULT_STATE_THREAD_STACK_SIZE, TX_NO_WAIT);
    }

    // create fault state thread
    // -> don't auto start it
    if (ret == TX_SUCCESS)
    {
        ret = tx_thread_create(&fault_state_thread, FAULT_STATE_THREAD_NAME, fault_state_thread_entry, 0, stack_ptr,
                    FAULT_STATE_THREAD_STACK_SIZE, FAULT_STATE_THREAD_PRIORITY, FAULT_STATE_THREAD_PREEMPTION_THRESHOLD,
                    TX_NO_TIME_SLICE, TX_DONT_START);
    }

    /*************************
     * Other initialisation
     **************************/

    // RTC timestamps
    if (ret == TX_SUCCESS)
    {
        rtc_time_init();
    }

    // message system
    if (ret == TX_SUCCESS)
    {
        ret = message_system_init();
    }

    if (ret == TX_SUCCESS)
    {
        pm100_status_t status = pm100_init();
        
        if (status != PM100_OK)
        {
            ret = TX_START_ERROR;
        }
    }


    /*************************
     * Ready-to-drive wait
     **************************/
    if (ret == TX_SUCCESS)
    {
        wait_for_ready_to_drive();
    }
    else 
    {
        tx_thread_resume(&fault_state_thread);
    }

    /*************************
     * Post ready-to-drive
     **************************/
    if (ret == TX_SUCCESS)
    {
        ret = can_rx_init();
    }

    /*************************
     * Start TraceX
     **************************/
    #if TRACEX_ENABLE
    if (ret == TX_SUCCESS)
    {
        ret = trace_init();
        trace_log_event(TRACE_READY_TO_DRIVE_EVENT, 0, 0, 0, 0);
    }
    #endif

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
