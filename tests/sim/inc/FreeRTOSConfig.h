#include <limits.h>
/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    FreeRTOSConfig.h
 * @brief   FreeRTOS kernel configuration for the VCU (STM32F746, Cortex-M7)
 *
 * @details Notes on the port from ThreadX:
 *          - Tick rate is 1 kHz, the same as TX_TIMER_TICKS_PER_SECOND was,
 *            so all tick-based timings in config.c are unchanged.
 *          - FreeRTOS priorities are the opposite way round to ThreadX
 *            (higher number = more urgent). configMAX_PRIORITIES is 56 so the
 *            CMSIS-RTOS2 priority range (used by RTCAN) is valid.
 *          - Time slicing is disabled to match the ThreadX configuration
 *            (TX_NO_TIME_SLICE): equal-priority tasks only switch when the
 *            running task blocks or yields.
 *          - ThreadX mutexes are recursive, FreeRTOS mutexes (as used by the
 *            services) are not: a task must not take a mutex it already
 *            holds, and only the holder may give it back.
 *          - The HAL timebase is TIM6; SysTick is used by FreeRTOS. The
 *            SysTick_Handler is provided by cmsis_os2.c.
 ******************************************************************************
 */
/* USER CODE END Header */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* Ensure definitions are only used by the compiler, and not by the assembler. */
#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
#include <stdint.h>
extern uint32_t SystemCoreClock;
#endif


/*
 * scheduler
 */
#define configUSE_PREEMPTION                     1
#define configUSE_TIME_SLICING                   0
#define configUSE_PORT_OPTIMISED_TASK_SELECTION  0
#define configUSE_TICKLESS_IDLE                  0
#define configCPU_CLOCK_HZ                       (216000000UL)
#define configTICK_RATE_HZ                       ((TickType_t)1000)
#define configMAX_PRIORITIES                     (56)
#define configMINIMAL_STACK_SIZE                 ((uint16_t)128)
#define configMAX_TASK_NAME_LEN                  (16)
#define configTICK_TYPE_WIDTH_IN_BITS            TICK_TYPE_WIDTH_32_BITS
#define configIDLE_SHOULD_YIELD                  1
#define configUSE_TASK_NOTIFICATIONS             1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES    1
#define configENABLE_BACKWARD_COMPATIBILITY      0
#define configUSE_NEWLIB_REENTRANT               0
#define configRECORD_STACK_HIGH_ADDRESS          1

/*
 * memory
 */
#define configSUPPORT_STATIC_ALLOCATION          1
#define configSUPPORT_DYNAMIC_ALLOCATION         1
#define configTOTAL_HEAP_SIZE                    ((size_t)(4 * 1024 * 1024))
#define configAPPLICATION_ALLOCATED_HEAP         0
#define configKERNEL_PROVIDED_STATIC_MEMORY      1

/*
 * synchronisation primitives
 */
#define configUSE_MUTEXES                        1
#define configUSE_RECURSIVE_MUTEXES              1
#define configUSE_COUNTING_SEMAPHORES            1
#define configQUEUE_REGISTRY_SIZE                8
#define configUSE_QUEUE_SETS                     0

/*
 * hooks and debugging
 */
#define configUSE_IDLE_HOOK                      0
#define configUSE_TICK_HOOK                      0
#define configUSE_MALLOC_FAILED_HOOK             1
#define configCHECK_FOR_STACK_OVERFLOW           0
#define configUSE_TRACE_FACILITY                 1
#define configUSE_STATS_FORMATTING_FUNCTIONS     0
#define configGENERATE_RUN_TIME_STATS            0

/*
 * co-routines (unused)
 */
#define configUSE_CO_ROUTINES                    0
#define configMAX_CO_ROUTINE_PRIORITIES          (2)

/*
 * software timers
 *
 * The ThreadX timer thread ran at the highest priority (0), so the FreeRTOS
 * timer service task does too.
 */
#define configUSE_TIMERS                         1
#define configTIMER_TASK_PRIORITY                (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH                 10
#define configTIMER_TASK_STACK_DEPTH             256

/*
 * CMSIS-RTOS2 wrapper (used by the RTCAN OSAL)
 */
#define USE_CUSTOM_SYSTICK_HANDLER_IMPLEMENTATION 0

/*
 * optional API functions
 */
#define INCLUDE_vTaskPrioritySet                 1
#define INCLUDE_uxTaskPriorityGet                1
#define INCLUDE_vTaskDelete                      1
#define INCLUDE_vTaskSuspend                     1
#define INCLUDE_xTaskDelayUntil                  1
#define INCLUDE_vTaskDelay                       1
#define INCLUDE_xTaskGetSchedulerState           1
#define INCLUDE_xTimerPendFunctionCall           1
#define INCLUDE_xQueueGetMutexHolder             1
#define INCLUDE_xSemaphoreGetMutexHolder         1
#define INCLUDE_uxTaskGetStackHighWaterMark      1
#define INCLUDE_xTaskGetCurrentTaskHandle        1
#define INCLUDE_eTaskGetState                    1
#define INCLUDE_xTaskAbortDelay                  1

/* host simulation specifics */
#include <stdio.h>
#include <stdlib.h>
#define xPortIsInsideInterrupt() pdFALSE
#define configASSERT(x) if ((x) == 0) { printf("ASSERT FAILED %s:%d\n", __FILE__, __LINE__); fflush(stdout); abort(); }
#endif /* FREERTOS_CONFIG_H */
