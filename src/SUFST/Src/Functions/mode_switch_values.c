#include "mode_switch_values.h"

ctrl_mode_t convert_mode_adc_to_discrete(uint16_t adc_reading_0_330)
{
    // LOG_INFO("MODE ADC %d\n", adc_reading_0_330);
    if (adc_reading_0_330 < 19)
        return 1;
    if (adc_reading_0_330 < 56)
        return 2;
    if (adc_reading_0_330 < 93)
        return 3;
    if (adc_reading_0_330 < 130)
        return 4;
    if (adc_reading_0_330 < 168)
        return 5;
    if (adc_reading_0_330 < 206)
        return 6;
    if (adc_reading_0_330 < 243)
        return 7;
    if (adc_reading_0_330 < 281)
        return 8;
    if (adc_reading_0_330 < 315)
        return 9;
    return 10;
}
