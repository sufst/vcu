/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    FreeRTOSConfig.h
 * @brief   FreeRTOS kernel configuration for the VCU (STM32F746, Cortex-M7)
 *
 * @details The generated part of this file follows the FREERTOS settings in
 *          VCU.ioc. Settings CubeMX does not manage are in the USER CODE
 *          sections ("Defines" at the end), so they survive regeneration.
 *
 *          Notes on the port from ThreadX:
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

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * These parameters and more are described within the 'configuration' section
 * of the FreeRTOS API documentation available on the FreeRTOS.org web site.
 *
 * See http://www.freertos.org/a00110.html
 *----------------------------------------------------------*/

/* USER CODE BEGIN Includes */
/* Section where include file can be added */
/* USER CODE END Includes */

/* Ensure definitions are only used by the compiler, and not by the assembler. */
#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
#include <stdint.h>
extern uint32_t SystemCoreClock;
#endif
#ifndef CMSIS_device_header
#define CMSIS_device_header "stm32f7xx.h"
#endif /* CMSIS_device_header */

#define configUSE_PREEMPTION                     1
#define configSUPPORT_STATIC_ALLOCATION          1
#define configSUPPORT_DYNAMIC_ALLOCATION         1
#define configUSE_IDLE_HOOK                      0
#define configUSE_TICK_HOOK                      0
#define configCPU_CLOCK_HZ                       (SystemCoreClock)
#define configTICK_RATE_HZ                       ((TickType_t)1000)
#define configMAX_PRIORITIES                     (56)
#define configMINIMAL_STACK_SIZE                 ((uint16_t)256)
#define configTOTAL_HEAP_SIZE                    ((size_t)65536)
#define configMAX_TASK_NAME_LEN                  (16)
#define configUSE_TRACE_FACILITY                 1
#define configUSE_16_BIT_TICKS                   0
#define configIDLE_SHOULD_YIELD                  1
#define configUSE_MUTEXES                        1
#define configQUEUE_REGISTRY_SIZE                8
#define configCHECK_FOR_STACK_OVERFLOW           2
#define configUSE_RECURSIVE_MUTEXES              1
#define configUSE_MALLOC_FAILED_HOOK             1
#define configUSE_COUNTING_SEMAPHORES            1
#define configRECORD_STACK_HIGH_ADDRESS          1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION  0
/* USER CODE BEGIN MESSAGE_BUFFER_LENGTH_TYPE */
/* Defaults to size_t for backward compatibility, but can be changed
   if lengths will always be less than the number of bytes in a size_t. */
#define configMESSAGE_BUFFER_LENGTH_TYPE         size_t
/* USER CODE END MESSAGE_BUFFER_LENGTH_TYPE */

/* Co-routine definitions. */
#define configUSE_CO_ROUTINES                    0
#define configMAX_CO_ROUTINE_PRIORITIES          (2)

/* Software timer definitions. */
#define configUSE_TIMERS                         1
#define configTIMER_TASK_PRIORITY                (55)
#define configTIMER_QUEUE_LENGTH                 10
#define configTIMER_TASK_STACK_DEPTH             256

/* CMSIS-RTOS V2 flags */
#define configUSE_OS2_THREAD_SUSPEND_RESUME      1
#define configUSE_OS2_THREAD_ENUMERATE           1
#define configUSE_OS2_EVENTFLAGS_FROM_ISR        1
#define configUSE_OS2_THREAD_FLAGS               1
#define configUSE_OS2_TIMER                      1
#define configUSE_OS2_MUTEX                      1

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */
#define INCLUDE_vTaskPrioritySet                 1
#define INCLUDE_uxTaskPriorityGet                1
#define INCLUDE_vTaskDelete                      1
#define INCLUDE_vTaskCleanUpResources            0
#define INCLUDE_vTaskSuspend                     1
#define INCLUDE_vTaskDelayUntil                  1
#define INCLUDE_vTaskDelay                       1
#define INCLUDE_xTaskGetSchedulerState           1
#define INCLUDE_xTimerPendFunctionCall           1
#define INCLUDE_xQueueGetMutexHolder             1
#define INCLUDE_uxTaskGetStackHighWaterMark      1
#define INCLUDE_xTaskGetCurrentTaskHandle        1
#define INCLUDE_eTaskGetState                    1
#define INCLUDE_xTaskAbortDelay                  1

/*
 * The CMSIS-RTOS V2 FreeRTOS wrapper is dependent on the heap implementation used
 * by the application thus the correct define need to be enabled below
 */
#define USE_FreeRTOS_HEAP_4

/* Cortex-M specific definitions. */
#ifdef __NVIC_PRIO_BITS
/* __BVIC_PRIO_BITS will be specified when CMSIS is being used. */
#define configPRIO_BITS __NVIC_PRIO_BITS
#else
#define configPRIO_BITS 4
#endif

/* The lowest interrupt priority that can be used in a call to a "set priority"
function. */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY      15

/* The highest interrupt priority that can be used by any interrupt service
routine that makes calls to interrupt safe FreeRTOS API functions.  DO NOT CALL
INTERRUPT SAFE FREERTOS API FUNCTIONS FROM ANY INTERRUPT THAT HAS A HIGHER
PRIORITY THAN THIS! (higher priorities are lower numeric values. */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

/* Interrupt priorities used by the kernel port layer itself.  These are generic
to all Cortex-M ports, and do not rely on any particular library functions. */
#define configKERNEL_INTERRUPT_PRIORITY \
    (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
/* !!!! configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to zero !!!!
See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* Normal assert() semantics without relying on the provision of an assert.h
header file. */
/* USER CODE BEGIN 1 */
/* a failed assert switches the outputs to a safe state (TS off) and blinks
   the heartbeat LED, see Error_Handler() in main.c */
#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
void Error_Handler(void);
#endif
#define configASSERT(x)    \
    if ((x) == 0)          \
    {                      \
        Error_Handler();   \
    }
/* USER CODE END 1 */

/* Definitions that map the FreeRTOS port interrupt handlers to their CMSIS
standard names. */
#define vPortSVCHandler    SVC_Handler
#define xPortPendSVHandler PendSV_Handler

/* IMPORTANT: After 10.3.1 update, Systick_Handler comes from NVIC (if SYS timebase = systick), otherwise from cmsis_os2.c */

#define USE_CUSTOM_SYSTICK_HANDLER_IMPLEMENTATION 0

/* USER CODE BEGIN Defines */
/* Section where parameter definitions can be added (for instance, to override default ones in FreeRTOS.h) */

/*
 * VCU settings CubeMX does not manage (or manages differently). Each one is
 * #undef'd first so that it wins whatever the generated part above contains.
 */

/* no time slicing, matching TX_NO_TIME_SLICE */
#undef configUSE_TIME_SLICING
#define configUSE_TIME_SLICING 0

/* 32-bit ticks (FreeRTOS V11 name for configUSE_16_BIT_TICKS = 0) */
#undef configUSE_16_BIT_TICKS
#undef configTICK_TYPE_WIDTH_IN_BITS
#define configTICK_TYPE_WIDTH_IN_BITS TICK_TYPE_WIDTH_32_BITS

/* the software timer task runs at the highest priority, like the ThreadX
   timer thread (priority 0) did */
#undef configTIMER_TASK_PRIORITY
#define configTIMER_TASK_PRIORITY (configMAX_PRIORITIES - 1)

#undef configUSE_TASK_NOTIFICATIONS
#define configUSE_TASK_NOTIFICATIONS 1
#undef configTASK_NOTIFICATION_ARRAY_ENTRIES
#define configTASK_NOTIFICATION_ARRAY_ENTRIES 1
#undef configENABLE_BACKWARD_COMPATIBILITY
#define configENABLE_BACKWARD_COMPATIBILITY 0
#undef configUSE_NEWLIB_REENTRANT
#define configUSE_NEWLIB_REENTRANT 0
#undef configUSE_TICKLESS_IDLE
#define configUSE_TICKLESS_IDLE 0
#undef configUSE_QUEUE_SETS
#define configUSE_QUEUE_SETS 0
#undef configUSE_STATS_FORMATTING_FUNCTIONS
#define configUSE_STATS_FORMATTING_FUNCTIONS 0
#undef configGENERATE_RUN_TIME_STATS
#define configGENERATE_RUN_TIME_STATS 0
#undef configAPPLICATION_ALLOCATED_HEAP
#define configAPPLICATION_ALLOCATED_HEAP 0
#undef configKERNEL_PROVIDED_STATIC_MEMORY
#define configKERNEL_PROVIDED_STATIC_MEMORY 1

/* optional API functions used by the services (wheelspeed uses
   xTaskDelayUntil, the mutex helpers use xSemaphoreGetMutexHolder) */
#undef INCLUDE_vTaskDelayUntil /* V11 name is INCLUDE_xTaskDelayUntil */
#undef INCLUDE_xTaskDelayUntil
#define INCLUDE_xTaskDelayUntil 1
#undef INCLUDE_xSemaphoreGetMutexHolder
#define INCLUDE_xSemaphoreGetMutexHolder 1
#undef INCLUDE_xTaskAbortDelay
#define INCLUDE_xTaskAbortDelay 1
#undef INCLUDE_uxTaskGetStackHighWaterMark
#define INCLUDE_uxTaskGetStackHighWaterMark 1

/*
 * Interrupt priorities: CAN (5), EXTI (6), SDMMC + SD DMA (7) and USB OTG (15)
 * call the RTOS "FromISR" API and are all at or below
 * configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY (5). The ADC and its DMA (0)
 * must not use the RTOS.
 */

/* USER CODE END Defines */

#endif /* FREERTOS_CONFIG_H */
