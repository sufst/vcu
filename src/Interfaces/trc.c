#include "trc.h"

#include <stdbool.h>

#include "io.h"

/**
 * @brief       Sets the state of the TS on connection to the TRC
 *
 * @details     The TS becomes ready when TS on is active and the TRC is not
 *              in a fault state. The TRC fault state is controlled by the
 *              inverter fault output.
 *
 * @param[in]   state   TS on pin state
 */
status_t trc_set_ts_on(GPIO_PinState state)
{
    return VCU_Output_Write(TS_ON_GPIO_Port, TS_ON_Pin, state);
}

bool trc_ready(void)
{
    return VCU_Input_Read(TS_READY_GPIO_Port, TS_READY_Pin);
}
