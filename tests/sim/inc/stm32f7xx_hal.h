/* Minimal fake STM32 HAL for host simulation of the VCU application */
#ifndef FAKE_STM32F7XX_HAL_H
#define FAKE_STM32F7XX_HAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define UNUSED(X) (void)(X)
#define HAL_MAX_DELAY 0xFFFFFFFFU

typedef enum
{
    HAL_OK = 0,
    HAL_ERROR = 1,
    HAL_BUSY = 2,
    HAL_TIMEOUT = 3
} HAL_StatusTypeDef;

typedef enum
{
    DISABLE = 0,
    ENABLE = 1
} FunctionalState;

/* GPIO */
typedef struct
{
    uint16_t odr;
    uint16_t idr;
    int id;
} GPIO_TypeDef;

extern GPIO_TypeDef sim_gpio[7];
#define GPIOA (&sim_gpio[0])
#define GPIOB (&sim_gpio[1])
#define GPIOC (&sim_gpio[2])
#define GPIOD (&sim_gpio[3])
#define GPIOE (&sim_gpio[4])
#define GPIOF (&sim_gpio[5])
#define GPIOG (&sim_gpio[6])

typedef enum
{
    GPIO_PIN_RESET = 0,
    GPIO_PIN_SET
} GPIO_PinState;

#define GPIO_PIN_0 ((uint16_t)0x0001)
#define GPIO_PIN_1 ((uint16_t)0x0002)
#define GPIO_PIN_2 ((uint16_t)0x0004)
#define GPIO_PIN_3 ((uint16_t)0x0008)
#define GPIO_PIN_4 ((uint16_t)0x0010)
#define GPIO_PIN_5 ((uint16_t)0x0020)
#define GPIO_PIN_6 ((uint16_t)0x0040)
#define GPIO_PIN_7 ((uint16_t)0x0080)
#define GPIO_PIN_8 ((uint16_t)0x0100)
#define GPIO_PIN_9 ((uint16_t)0x0200)
#define GPIO_PIN_10 ((uint16_t)0x0400)
#define GPIO_PIN_11 ((uint16_t)0x0800)
#define GPIO_PIN_12 ((uint16_t)0x1000)
#define GPIO_PIN_13 ((uint16_t)0x2000)
#define GPIO_PIN_14 ((uint16_t)0x4000)
#define GPIO_PIN_15 ((uint16_t)0x8000)

#define IS_GPIO_ALL_INSTANCE(p) ((p) >= &sim_gpio[0] && (p) <= &sim_gpio[6])
#define IS_GPIO_PIN(p) (((p) & 0xFFFFU) != 0U)

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin);
void HAL_GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state);
void HAL_GPIO_TogglePin(GPIO_TypeDef *port, uint16_t pin);

/* ADC */
typedef struct
{
    int id;
} ADC_TypeDef;
extern ADC_TypeDef sim_adc_inst1, sim_adc_inst2;
#define ADC1 (&sim_adc_inst1)
#define ADC2 (&sim_adc_inst2)
typedef struct
{
    ADC_TypeDef *Instance;
} ADC_HandleTypeDef;

/* UART / USART */
typedef struct
{
    int id;
} UART_HandleTypeDef;
typedef struct
{
    int id;
} USART_HandleTypeDef;
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *h, const uint8_t *data, uint16_t size, uint32_t timeout);
HAL_StatusTypeDef HAL_USART_Transmit(USART_HandleTypeDef *h, const uint8_t *data, uint16_t size, uint32_t timeout);

/* CAN */
typedef enum
{
    HAL_CAN_STATE_RESET = 0,
    HAL_CAN_STATE_READY = 1,
    HAL_CAN_STATE_LISTENING = 2,
} HAL_CAN_StateTypeDef;

typedef struct
{
    int id;
    uint32_t ErrorCode;
} CAN_HandleTypeDef;

typedef struct
{
    uint32_t FilterIdHigh;
    uint32_t FilterIdLow;
    uint32_t FilterMaskIdHigh;
    uint32_t FilterMaskIdLow;
    uint32_t FilterFIFOAssignment;
    uint32_t FilterBank;
    uint32_t FilterMode;
    uint32_t FilterScale;
    uint32_t FilterActivation;
    uint32_t SlaveStartFilterBank;
} CAN_FilterTypeDef;

typedef struct
{
    uint32_t StdId;
    uint32_t ExtId;
    uint32_t IDE;
    uint32_t RTR;
    uint32_t DLC;
    FunctionalState TransmitGlobalTime;
} CAN_TxHeaderTypeDef;

typedef struct
{
    uint32_t StdId;
    uint32_t ExtId;
    uint32_t IDE;
    uint32_t RTR;
    uint32_t DLC;
    uint32_t Timestamp;
    uint32_t FilterMatchIndex;
} CAN_RxHeaderTypeDef;

#define CAN_ID_STD 0x0U
#define CAN_ID_EXT 0x4U
#define CAN_RTR_DATA 0x0U
#define CAN_FILTER_FIFO0 0U
#define CAN_FILTER_ENABLE 1U
#define CAN_FILTERMODE_IDMASK 0U
#define CAN_FILTERSCALE_32BIT 1U
#define CAN_IT_TX_MAILBOX_EMPTY 0x1U
#define CAN_IT_RX_FIFO0_MSG_PENDING 0x2U
#define CAN_IT_RX_FIFO1_MSG_PENDING 0x10U
#define CAN_IT_ERROR 0x8000U
#define CAN_IT_BUSOFF 0x400U
#define CAN_IT_ERROR_PASSIVE 0x200U
#define CAN_IT_ERROR_WARNING 0x100U
#define HAL_CAN_ERROR_NONE 0x0U
#define HAL_CAN_ERROR_TX_ALST0 0x00000080U
#define HAL_CAN_ERROR_TX_TERR0 0x00000100U
#define HAL_CAN_ERROR_TX_ALST1 0x00000200U
#define HAL_CAN_ERROR_TX_TERR1 0x00000400U
#define HAL_CAN_ERROR_TX_ALST2 0x00000800U
#define HAL_CAN_ERROR_TX_TERR2 0x00001000U

HAL_StatusTypeDef HAL_CAN_ConfigFilter(CAN_HandleTypeDef *hcan, const CAN_FilterTypeDef *filter);
HAL_StatusTypeDef HAL_CAN_Start(CAN_HandleTypeDef *hcan);
HAL_StatusTypeDef HAL_CAN_ActivateNotification(CAN_HandleTypeDef *hcan, uint32_t its);
HAL_CAN_StateTypeDef HAL_CAN_GetState(const CAN_HandleTypeDef *hcan);
uint32_t HAL_CAN_GetError(const CAN_HandleTypeDef *hcan);
HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef *hcan, const CAN_TxHeaderTypeDef *header,
                                       const uint8_t data[], uint32_t *mailbox);
HAL_StatusTypeDef HAL_CAN_GetRxMessage(CAN_HandleTypeDef *hcan, uint32_t fifo,
                                       CAN_RxHeaderTypeDef *header, uint8_t data[]);

void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan);

void Error_Handler(void);

/* DWT cycle counter / core debug (wheelspeed edge timestamps): the counter runs
 * at SystemCoreClock off the host's monotonic clock, read through DWT->CYCCNT */
typedef struct
{
    volatile uint32_t CTRL, LAR, CYCCNT;
} DWT_Type;
typedef struct
{
    volatile uint32_t DEMCR;
} CoreDebug_Type;

extern uint32_t SystemCoreClock;
DWT_Type *sim_dwt(void);
extern CoreDebug_Type sim_coredebug;
#define DWT (sim_dwt())
#define CoreDebug (&sim_coredebug)
#define CoreDebug_DEMCR_TRCENA_Msk (1UL << 24)
#define DWT_CTRL_CYCCNTENA_Msk (1UL << 0)

#endif
