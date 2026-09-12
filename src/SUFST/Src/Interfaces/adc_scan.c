/***************************************************************************
 * @file   adc_scan.c
 * @brief  Free-running scan+DMA ADC sampling, with rolling-average readout
 ***************************************************************************/

#include "adc_scan.h"

#define ADC1_NUM_CHANNELS 5
#define ADC1_NUM_SCANS 35
#define ADC1_BUF_LEN (ADC1_NUM_CHANNELS * ADC1_NUM_SCANS)

#define ADC2_NUM_CHANNELS 1
#define ADC2_NUM_SCANS 174
#define ADC2_BUF_LEN (ADC2_NUM_CHANNELS * ADC2_NUM_SCANS)

/*
 * Written by DMA in circular mode, so always volatile from the CPU's point
 * of view. Placed in the .dma_bss linker section (see
 * STM32F746ZGTx_FLASH.ld) to guarantee they land in DMA-reachable SRAM
 * rather than DTCM, which DMA masters cannot access.
 *
 * D-cache is not enabled anywhere in this codebase (no SCB_EnableDCache()
 * call), so no cache maintenance is needed around these buffers. If that
 * ever changes, these buffers will need an MPU no-cache region or manual
 * SCB_InvalidateDCache_by_Addr() calls before each read below.
 */
static volatile uint16_t adc1_buf[ADC1_BUF_LEN]
    __attribute__((section(".dma_bss")));
static volatile uint16_t adc2_buf[ADC2_BUF_LEN]
    __attribute__((section(".dma_bss")));

static volatile bool adc1_primed = false;
static volatile bool adc2_primed = false;
static volatile bool adc1_faulted = false;
static volatile bool adc2_faulted = false;

/**
 * @brief   Starts the free-running ADC1/ADC2 scans
 *
 * @details Must be called once, after MX_ADC1_Init()/MX_ADC2_Init(). Readings
 *          are not valid until adc_scan_is_ready() returns true, ~1ms later.
 */
status_t adc_scan_init(void)
{
    status_t status = STATUS_OK;

    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc1_buf, ADC1_BUF_LEN) != HAL_OK)
    {
        status = STATUS_ERROR;
    }

    if (HAL_ADC_Start_DMA(&hadc2, (uint32_t *)adc2_buf, ADC2_BUF_LEN) != HAL_OK)
    {
        status = STATUS_ERROR;
    }

    return status;
}

/**
 * @brief       Rolling average of the last ~1ms of samples for one channel
 *
 * @param[in]   hadc    ADC instance (&hadc1 or &hadc2)
 * @param[in]   slot    0-based index into hadc's scan sequence (rank - 1)
 */
uint16_t adc_scan_get_average(const ADC_HandleTypeDef *hadc, uint8_t slot)
{
    const volatile uint16_t *buf;
    uint8_t stride;
    uint16_t num_scans;

    if (hadc->Instance == ADC2)
    {
        buf = adc2_buf;
        stride = ADC2_NUM_CHANNELS;
        num_scans = ADC2_NUM_SCANS;
    }
    else
    {
        buf = adc1_buf;
        stride = ADC1_NUM_CHANNELS;
        num_scans = ADC1_NUM_SCANS;
    }

    uint32_t sum = 0;

    for (uint16_t i = 0; i < num_scans; i++)
    {
        sum += buf[(i * stride) + slot];
    }

    return (uint16_t)(sum / num_scans);
}

/**
 * @brief   True once hadc has completed at least one full scan cycle and has
 *          not since faulted (e.g. DMA overrun)
 */
bool adc_scan_is_ready(const ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC2)
    {
        return adc2_primed && !adc2_faulted;
    }

    return adc1_primed && !adc1_faulted;
}

// trunk-ignore(cppcheck/constParameterPointer)
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        adc1_primed = true;
        adc1_faulted = false;
    }
    else if (hadc->Instance == ADC2)
    {
        adc2_primed = true;
        adc2_faulted = false;
    }
}

// trunk-ignore(cppcheck/constParameterPointer)
void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        adc1_faulted = true;
    }
    else if (hadc->Instance == ADC2)
    {
        adc2_faulted = true;
    }
}
