/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    freertos.c
 * @brief   FreeRTOS application initialisation and HAL callback routing
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "heartbeat.h"
#include "status.h"
#include "trc.h"
#include "vcu.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* stack of the task that runs vcu_init() (freed once it has finished) */
#define VCU_INIT_TASK_STACK_BYTES 2048U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/**
 * @brief   VCU instance
 */
static vcu_context_t vcu;

/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
static void vcu_init_task(void *arg);

/* USER CODE END FunctionPrototypes */

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);
void vApplicationMallocFailedHook(void);

/* USER CODE BEGIN 4 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    Error_Handler();
}
/* USER CODE END 4 */

/* USER CODE BEGIN 5 */
void vApplicationMallocFailedHook(void)
{
    Error_Handler();
}
/* USER CODE END 5 */

/**
 * @brief   FreeRTOS initialisation (called before the scheduler starts)
 *
 * @details All tasks, mutexes, queues and timers are created by the SUFST
 *          services from vcu_init(), none are configured in CubeMX.
 */
void MX_FREERTOS_Init(void)
{
    /* USER CODE BEGIN Init */
    if (xTaskCreate(vcu_init_task, "VCU init", VCU_INIT_TASK_STACK_BYTES / sizeof(StackType_t),
                    NULL, configMAX_PRIORITIES - 1, NULL) != pdPASS)
    {
        Error_Handler();
    }
    /* USER CODE END Init */

    /* USER CODE BEGIN RTOS_MUTEX */
    /* USER CODE END RTOS_MUTEX */

    /* USER CODE BEGIN RTOS_SEMAPHORES */
    /* USER CODE END RTOS_SEMAPHORES */

    /* USER CODE BEGIN RTOS_TIMERS */
    /* USER CODE END RTOS_TIMERS */

    /* USER CODE BEGIN RTOS_QUEUES */
    /* USER CODE END RTOS_QUEUES */

    /* USER CODE BEGIN RTOS_THREADS */
    /* USER CODE END RTOS_THREADS */

    /* USER CODE BEGIN RTOS_EVENTS */
    /* USER CODE END RTOS_EVENTS */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/**
 * @brief   Initialises the VCU services, then deletes itself
 *
 * @details This runs as the first task, at the highest priority, rather than
 *          before the scheduler starts. Before the scheduler starts FreeRTOS
 *          keeps interrupts masked, which stops the HAL tick (TIM6), so a HAL
 *          timeout inside the initialisation (e.g. HAL_CAN_Start() when a
 *          CAN transceiver has no power) would never expire and the VCU would
 *          hang at boot without any indication. The service tasks created
 *          here all have a lower priority, so none of them runs until the
 *          initialisation is complete.
 *
 *          If initialisation fails, the services which did start (e.g.
 *          logging) keep running, but the control task is suspended so the
 *          car can't be driven, the TS is kept off and the heartbeat LED
 *          blinks fast.
 */
static void vcu_init_task(void *arg)
{
    (void)arg;

    if (vcu_init(&vcu, &hcan2, &hcan1, config_get()) != STATUS_OK)
    {
        // don't run the control task on top of half-initialised services
        // (e.g. a service whose mutex was never created would hit
        // configASSERT)
        if (vcu.ctrl.thread != NULL)
        {
            vTaskSuspend(vcu.ctrl.thread);
        }
        trc_set_ts_on(GPIO_PIN_RESET);
        heartbeat_set_fast();
    }

    vTaskDelete(NULL);
}

/*
 * These callbacks are put here so that all "global" effects are contained
 * within this file, allowing all the SUFST code to be modular.
 */
void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *can_h)
{
    vcu_handle_can_tx_mailbox_callback(&vcu, can_h);
}

void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *can_h)
{
    vcu_handle_can_tx_mailbox_callback(&vcu, can_h);
}

void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *can_h)
{
    vcu_handle_can_tx_mailbox_callback(&vcu, can_h);
}

void HAL_CAN_TxMailbox0AbortCallback(CAN_HandleTypeDef *can_h)
{
    vcu_handle_can_tx_mailbox_callback(&vcu, can_h);
}

void HAL_CAN_TxMailbox1AbortCallback(CAN_HandleTypeDef *can_h)
{
    vcu_handle_can_tx_mailbox_callback(&vcu, can_h);
}

void HAL_CAN_TxMailbox2AbortCallback(CAN_HandleTypeDef *can_h)
{
    vcu_handle_can_tx_mailbox_callback(&vcu, can_h);
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *can_h)
{
    vcu_handle_can_rx_it(&vcu, can_h, 0);
}

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *can_h)
{
    vcu_handle_can_rx_it(&vcu, can_h, 1);
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *can_h)
{
    vcu_handle_can_err(&vcu, can_h);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    vcu_handle_gpio_exti(&vcu, GPIO_Pin);
}

/* USER CODE END Application */
