#include "io.h"

/**
 * @brief       Sets the specified GPIO pin(s) to low, so the output will be
 * high, due to the io protection on the STAG 12 VCU
 *
 * @param[in]   port  GPIO port definition
 * @param[in]   pin   GPIO pin number(s)
 */
status_t VCU_Output_High(GPIO_TypeDef *port, uint16_t pin)
{
    if (IS_GPIO_ALL_INSTANCE(port) && IS_GPIO_PIN(pin))
    {
        HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
        return STATUS_OK;
    }
    return STATUS_ERROR;
}

/**
 * @brief       Sets the specified GPIO pin(s) to high, so the output will be
 * low, due to the io inversion on the STAG 12 VCU
 *
 * @param[in]   port  GPIO port definition
 * @param[in]   pin   GPIO pin number(s)
 */
status_t VCU_Output_Low(GPIO_TypeDef *port, uint16_t pin)
{
    if (IS_GPIO_ALL_INSTANCE(port) && IS_GPIO_PIN(pin))
    {
        HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
        return STATUS_OK;
    }
    return STATUS_ERROR;
}

/**
 * @brief       Sets the specified GPIO pin(s) based on the given state
 *
 * @param[in]   port  GPIO port definition
 * @param[in]   pin   GPIO pin number(s)
 * @param[in]   state Desired state (GPIO_PIN_SET or GPIO_PIN_RESET)
 */
status_t VCU_Output_Write(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state)
{
    if (state == GPIO_PIN_RESET)
    {
        return VCU_Output_Low(port, pin);
    }
    else if (state == GPIO_PIN_SET)
    {
        return VCU_Output_High(port, pin);
    }
    return STATUS_ERROR;
}

/**
 * @brief       Toggles the specified GPIO pin(s)
 *
 * @param[in]   port  GPIO port definition
 * @param[in]   pin   GPIO pin number(s)
 */
status_t VCU_Output_Toggle(GPIO_TypeDef *port, uint16_t pin)
{
    if (IS_GPIO_ALL_INSTANCE(port) && IS_GPIO_PIN(pin))
    {
        HAL_GPIO_TogglePin(port, pin);
        return STATUS_OK;
    }
    return STATUS_ERROR;
}

/**
 * @brief       Reads the logical state of the specified GPIO pin(s) based on STAG 12 VCU logic level inversion
 * @param[in]   port  GPIO port definition
 * @param[in]   pin   GPIO pin number(s)
 */
bool VCU_Input_Read(GPIO_TypeDef *port, uint16_t pin)
{
    if (!IS_GPIO_ALL_INSTANCE(port) || !IS_GPIO_PIN(pin))
    {
        return false;
    }
    GPIO_PinState pin_state = HAL_GPIO_ReadPin(port, pin);
    return !pin_state;
}
