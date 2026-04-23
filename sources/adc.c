#include "adc.h"

struct wiringPiNodeStruct *adc_node;
int calibration = 0;

void init_ads1115(void)
{
    // Initialise ADC
    if (ads1115Setup(&adc_node, PIN_BASE, DEVICE_ID) == 0)
    {
        syslog(LOG_PERROR, "Could not initialize ADS1115 - Photoresistor cannot be read\n");
        return;
    }
}

void calibrate_ads1115(void)
{
    calibration = adc_node->analogRead(adc_node, 0);
}

void capture_ads1115(uint16_t **buffer, int b_len)
{
    for (int i = 0; i < b_len; i++)
    {
        (*buffer)[i] = (uint16_t)adc_node->analogRead(adc_node, 0);
    }
}

uint16_t read_ads1115(void)
{
    return (uint16_t)adc_node->analogRead(adc_node, 0);
}
