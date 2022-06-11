/***************************************************************************
 * @file   drs.c
 * @author Chua Shen Yik (syc2e18@soton.ac.uk)
 * @date   2022-06-08
 * @brief  DRS communication implementation
 ***************************************************************************/

#include "drs.h"

#include "config.h"
#include "tx_api.h"

#include "can_device_state.h"
#include "can_message.h"

#define DRS_STATE_MUTEX_NAME "DRS State Mutex"

/**
 * @brief DRS state
 */
static uint32_t DRS_state;

/**
 * @brief State mutex
 */
static TX_MUTEX DRS_state_mutex;

/**
 * @brief Initialise drs CAN input and DAC output 
 */
drs_status_t drs_init()
{
    // create state mutex
    drs_status_t status = DRS_OK;
    if (tx_mutex_create(&DRS_state_mutex, DRS_STATE_MUTEX_NAME, TX_NO_INHERIT)
        != TX_SUCCESS)
    {
        status = DRS_ERROR;
    }

    // reset state
    memset(&DRS_state, 0x0000, sizeof(DRS_state));

    if(HAL_DAC_Start(&DRS_DAC_HANDLE, DAC_CHANNEL_1) != HAL_OK){
        status = (status == DRS_OK)? DAC_ERROR:DRS_DAC_ERROR;
    }
    DRS_state = 0;
    if(set_dac_val() != DRS_OK){
        status = (status == DRS_OK)? DAC_ERROR:DRS_DAC_ERROR;
    }       

    return status;
}

/**
 * @brief       Thread-safe drs state read
 *
 * @param[out]  value_ptr   Pointer to location to store fetched data
 */
drs_status_t drs_read_state(uint32_t* value_ptr)
{
    drs_status_t status = DRS_OK;

    if (value_ptr != NULL)
    {
        tx_mutex_get(&DRS_state_mutex, TX_WAIT_FOREVER);
        *value_ptr = DRS_state;
        tx_mutex_put(&DRS_state_mutex);
    }

    return status;
}

/**
 * @brief       Thread-safe drs state update
 *
 * @note        Index parameter unused but required to match standard function
 * signature for CAN state setters
 *
 * @param[in]   index   Unused
 * @param[in]   value   Value to write to state
 */
void drs_update_state(uint32_t index, uint32_t value)
{
    (void) index; // unused

    if (tx_mutex_get(&DRS_state_mutex, TX_WAIT_FOREVER) == TX_SUCCESS)
    {
        DRS_state = value;
        tx_mutex_put(&DRS_state_mutex);
    }
}

/**
 * @brief       Set dac value based on DRS state
 *
 * @param[in]   DRS_state   The state of DRS (on/off)
 * 
 * @return     The drs_status of setting dac value.
 */
drs_status_t set_dac_val()
{
    drs_status_t status = DRS_OK;
    if(DRS_state == 0){
        if(HAL_DAC_SetValue(&DRS_DAC_HANDLE, DAC_CHANNEL_1,DAC_ALIGN_12B_R,(uint32_t)DRS_OFF_VALUE) != HAL_OK){
            status = DAC_ERROR;
        }
    }
    else {
        if(HAL_DAC_SetValue(&DRS_DAC_HANDLE, DAC_CHANNEL_1,DAC_ALIGN_12B_R,(uint32_t)DRS_ON_VALUE) != HAL_OK){
            status = DAC_ERROR;
        }
    }
    return status;
}