#include "bps.h"

// /**
//  * @brief Safety critical signal instance for BPS
//  */
// static scs_t bps_signal;

// /**
//  * @brief Initialise BPS
//  */
// void bps_init()
// {
//     // TODO: reimplement
//     // uint32_t bps_signal_max = (1 << BPS_SCALED_RESOLUTION) - 1;

//     // scs_create(&bps_signal,
//     //            &hadc3,
//     //            BPS_ADC_MIN,
//     //            BPS_ADC_MAX,
//     //            0,
//     //            bps_signal_max);
// }

// /**
//  * @brief Read BPS
//  */
// uint32_t bps_read()
// {
//     // TODO: reimplement
//     // uint32_t reading = scs_read(&bps_signal);

// #if !BPS_DISABLE_SCS_CHECK
//     if (!scs_validate(&bps_signal))
//     {
//         // TODO: create fault handling system
//         Error_Handler();
//     }
// #endif

//     return 0;
// }

// /**
//  * @brief   Checks if BPS is fully pressed
//  *
//  * @return  true    BPS is fully pressed
//  * @return  false   BPS is not fully pressed
//  */
// bool bps_fully_pressed()
// {
//     const uint32_t bps_max = (1 << BPS_SCALED_RESOLUTION) - 1;
//     const uint32_t bps_threshold = bps_max * BPS_FULLY_PRESSED_THRESHOLD;

//     uint32_t bps_input = bps_read();

//     return bps_input > bps_threshold;
// }
