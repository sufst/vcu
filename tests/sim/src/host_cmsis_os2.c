/* Minimal CMSIS-RTOS2 shim over FreeRTOS for the host simulation (only what
 * the RTCAN FreeRTOS OSAL uses). On the target, ST's cmsis_os2.c is used. */
#include <FreeRTOS.h>
#include <cmsis_os2.h>
#include <queue.h>
#include <semphr.h>
#include <task.h>

osThreadId_t osThreadNew(osThreadFunc_t func, void *argument, const osThreadAttr_t *attr)
{
    TaskHandle_t h = NULL;
    uint32_t stack = (attr && attr->stack_size) ? attr->stack_size / sizeof(StackType_t) : configMINIMAL_STACK_SIZE;
    UBaseType_t prio = (attr && attr->priority) ? (UBaseType_t)attr->priority : 24;
    configASSERT(prio < configMAX_PRIORITIES);
    if (xTaskCreate((TaskFunction_t)func, attr ? attr->name : "os", stack, argument, prio, &h) != pdPASS)
        return NULL;
    return (osThreadId_t)h;
}

osStatus_t osThreadYield(void)
{
    taskYIELD();
    return osOK;
}

osMessageQueueId_t osMessageQueueNew(uint32_t count, uint32_t size, const osMessageQueueAttr_t *attr)
{
    if (attr && attr->cb_mem && attr->mq_mem)
        return (osMessageQueueId_t)xQueueCreateStatic(count, size, attr->mq_mem, attr->cb_mem);
    return (osMessageQueueId_t)xQueueCreate(count, size);
}

osStatus_t osMessageQueuePut(osMessageQueueId_t mq, const void *msg, uint8_t prio, uint32_t timeout)
{
    (void)prio;
    TickType_t t = (timeout == osWaitForever) ? portMAX_DELAY : timeout;
    if (xQueueSendToBack((QueueHandle_t)mq, msg, t) == pdPASS)
        return osOK;
    return timeout ? osErrorTimeout : osErrorResource;
}

osStatus_t osMessageQueueGet(osMessageQueueId_t mq, void *msg, uint8_t *prio, uint32_t timeout)
{
    (void)prio;
    TickType_t t = (timeout == osWaitForever) ? portMAX_DELAY : timeout;
    if (xQueueReceive((QueueHandle_t)mq, msg, t) == pdPASS)
        return osOK;
    return timeout ? osErrorTimeout : osErrorResource;
}

osStatus_t osMessageQueueDelete(osMessageQueueId_t mq)
{
    vQueueDelete((QueueHandle_t)mq);
    return osOK;
}

osSemaphoreId_t osSemaphoreNew(uint32_t max_count, uint32_t initial_count, const osSemaphoreAttr_t *attr)
{
    (void)attr;
    if (max_count == 1)
    {
        SemaphoreHandle_t s = xSemaphoreCreateBinary();
        if (s && initial_count)
            xSemaphoreGive(s);
        return (osSemaphoreId_t)s;
    }
    return (osSemaphoreId_t)xSemaphoreCreateCounting(max_count, initial_count);
}

osStatus_t osSemaphoreAcquire(osSemaphoreId_t s, uint32_t timeout)
{
    TickType_t t = (timeout == osWaitForever) ? portMAX_DELAY : timeout;
    if (xSemaphoreTake((SemaphoreHandle_t)s, t) == pdPASS)
        return osOK;
    return timeout ? osErrorTimeout : osErrorResource;
}

osStatus_t osSemaphoreRelease(osSemaphoreId_t s)
{
    return (xSemaphoreGive((SemaphoreHandle_t)s) == pdPASS) ? osOK : osErrorResource;
}
