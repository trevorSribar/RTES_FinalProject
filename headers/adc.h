#include "ads1x1x.h"
#include "generic.h"

#define DEVICE_ID 0xFF //to be determined through actual testing

//Initialize the ADS1115 device
void init_ads1115(void);

//Calibrate the ADS1115 device to environment
void calibrate_ads1115(void);

//Capture <b_len> ADS1115 inputs and return a buffer containing them
uint16_t *capture_ads1115(int b_len);
