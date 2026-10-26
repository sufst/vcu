/* Fake HAL for host simulation: GPIO, UART, CAN bus model */
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <FreeRTOS.h>
#include <task.h>

#include "main.h"
#include "sim.h"

uint32_t SystemCoreClock = 216000000U;
CoreDebug_Type sim_coredebug;

DWT_Type *sim_dwt(void)
{
    static DWT_Type dwt;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t ns = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    dwt.CYCCNT = (uint32_t)(ns * (SystemCoreClock / 1000000U) / 1000U);
    return &dwt;
}

GPIO_TypeDef sim_gpio[7] = {
    { 0, 0xFFFF, 0 }, { 0, 0xFFFF, 1 }, { 0, 0xFFFF, 2 }, { 0, 0xFFFF, 3 },
    { 0, 0xFFFF, 4 }, { 0, 0xFFFF, 5 }, { 0, 0xFFFF, 6 },
};
ADC_TypeDef sim_adc_inst1 = { 1 }, sim_adc_inst2 = { 2 };
ADC_HandleTypeDef hadc1 = { &sim_adc_inst1 }, hadc2 = { &sim_adc_inst2 };
CAN_HandleTypeDef hcan1 = { 1, 0 }, hcan2 = { 2, 0 };
UART_HandleTypeDef huart8 = { 8 };
USART_HandleTypeDef husart1 = { 1 };

int sim_quiet_log = 0;

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin)
{
    return (port->idr & pin) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state)
{
    if (state == GPIO_PIN_SET)
        port->odr |= pin;
    else
        port->odr &= (uint16_t)~pin;
}

void HAL_GPIO_TogglePin(GPIO_TypeDef *port, uint16_t pin)
{
    port->odr ^= pin;
}

void sim_set_input(GPIO_TypeDef *port, uint16_t pin, int level)
{
    if (level)
        port->idr |= pin;
    else
        port->idr &= (uint16_t)~pin;
}

int sim_get_output(GPIO_TypeDef *port, uint16_t pin)
{
    return (port->odr & pin) != 0;
}

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *h, const uint8_t *data, uint16_t size, uint32_t timeout)
{
    (void)h;
    (void)timeout;
    if (!sim_quiet_log && !strstr((const char *)data, "ADC: ") && !strstr((const char *)data, "Sending torque"))
    {
        printf("    [uart] %.*s", (int)size, (const char *)data);
        if (size == 0 || data[size - 1] != '\n')
            printf("\n");
        fflush(stdout);
    }
    sim_log_lines++;
    if (!strstr((const char *)data, "RAM: "))
        sim_log_lines_no_ram++;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_USART_Transmit(USART_HandleTypeDef *h, const uint8_t *data, uint16_t size, uint32_t timeout)
{
    (void)h;
    (void)data;
    (void)size;
    (void)timeout;
    return HAL_OK;
}

/*
 * CAN
 */
#define RX_RING 64
typedef struct
{
    uint32_t id;
    uint8_t len;
    uint8_t data[8];
} frame_t;

static frame_t rx_ring[3][RX_RING];
static unsigned rx_head[3], rx_tail[3];

volatile unsigned sim_tx_count[3];
volatile int sim_last_torque_cmd = -1;
volatile int sim_last_inverter_enable = -1;
volatile unsigned sim_torque_cmd_count;
volatile unsigned sim_disable_cmd_count;
volatile unsigned sim_log_lines;
volatile unsigned sim_log_lines_no_ram; // excluding the periodic RAM report

HAL_StatusTypeDef HAL_CAN_ConfigFilter(CAN_HandleTypeDef *hcan, const CAN_FilterTypeDef *f)
{
    (void)hcan;
    (void)f;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_CAN_Start(CAN_HandleTypeDef *hcan)
{
    (void)hcan;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_CAN_ActivateNotification(CAN_HandleTypeDef *hcan, uint32_t its)
{
    (void)hcan;
    (void)its;
    return HAL_OK;
}

HAL_CAN_StateTypeDef HAL_CAN_GetState(const CAN_HandleTypeDef *hcan)
{
    (void)hcan;
    return HAL_CAN_STATE_LISTENING;
}

uint32_t HAL_CAN_GetError(const CAN_HandleTypeDef *hcan)
{
    return hcan->ErrorCode;
}

HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef *hcan, const CAN_TxHeaderTypeDef *header,
                                       const uint8_t data[], uint32_t *mailbox)
{
    *mailbox = 1;
    sim_tx_count[hcan->id]++;
    uint32_t id = (header->IDE == CAN_ID_EXT) ? header->ExtId : header->StdId;
    sim_on_tx(hcan->id, id, data, (uint8_t)header->DLC);

    // transmission completes immediately
    HAL_CAN_TxMailbox0CompleteCallback(hcan);
    return HAL_OK;
}

HAL_StatusTypeDef HAL_CAN_GetRxMessage(CAN_HandleTypeDef *hcan, uint32_t fifo,
                                       CAN_RxHeaderTypeDef *header, uint8_t data[])
{
    (void)fifo;
    int b = hcan->id;
    if (rx_head[b] == rx_tail[b])
        return HAL_ERROR;
    frame_t *f = &rx_ring[b][rx_tail[b] % RX_RING];
    rx_tail[b]++;
    memset(header, 0, sizeof(*header));
    header->IDE = CAN_ID_STD;
    header->StdId = f->id;
    header->DLC = f->len;
    memcpy(data, f->data, 8);
    return HAL_OK;
}

void sim_push_rx(int bus, uint32_t id, const uint8_t *data, uint8_t len)
{
    frame_t *f = &rx_ring[bus][rx_head[bus] % RX_RING];
    f->id = id;
    f->len = len;
    memset(f->data, 0, 8);
    memcpy(f->data, data, len);
    rx_head[bus]++;
}

void Error_Handler(void)
{
    printf("ERROR_HANDLER CALLED\n");
    fflush(stdout);
    abort();
}
