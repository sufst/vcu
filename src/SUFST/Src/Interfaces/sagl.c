/***************************************************************************
 * @file   sagl.c
 * @author Dmytro Avdieienko (@Avdieienko)
 * @brief  Steering Wheel Angle Sensor
 ***************************************************************************/

#include "sagl.h"

/**
 * @brief       Initialises BPS
 *
 * @param[in]   bps_ptr     BPS context
 * @param[in]   config_ptr  Configuration
 */
status_t sagl_init(sagl_context_t* sagl_ptr, const config_sagl_t* config_ptr)
{
    sagl_ptr->config_ptr = config_ptr;

    // compute pressure thresholds
    const uint16_t range
        = config_ptr->scs.max_mapped - config_ptr->scs.min_mapped;

    const uint16_t offset = config_ptr->scs.min_mapped;

    // create the SCS instance
    status_t status = scs_create(&sagl_ptr->signal, &config_ptr->scs);

    return status;
}

status_t sagl_read(sagl_context_t* sagl_ptr, uint16_t* reading_ptr)
{
  scs_t scs_ptr = sagl_ptr->signal;
  config_scs_t* config_ptr = sagl_ptr->config_ptr;
  
  if(HAL_ADC_ConfigChannel(&config_ptr->hadc, &config_ptr->channel_config) != HAL_OK)
  {
    return STATUS_ERROR;
  }
  return scs_read(&sagl_ptr->signal, reading_ptr);
}