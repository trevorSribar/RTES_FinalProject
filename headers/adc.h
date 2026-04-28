// Exposes the functions to interact with the ADS1115 ADC and obtain readings
// Kenneth Alcineus
// 4/18/2026

#include "ads1115.h"
#include "generic.h"

#define DEVICE_ID 0x48
#define PIN_BASE 200
#define ADC_BASIS1_PHOTOSENSOR_READ_HIGH 10000
#define ADC_BASIS2_PHOTOSENSOR_READ_HIGH 1200

//Initialize the ADS1115 device 
void init_ads1115(void);

//Calibrate the ADS1115 device to environment
void calibrate_ads1115(void);

//Capture <b_len> ADS1115 readings into a buffer containing them
void capture_ads1115(uint16_t **buffer, int b_len);

//Capture a single ADS1115 reading and return it
uint16_t read_ads1115(void);

//Capture a single ADS1115 reading with the calibration as a regular integer
int read_calibrated(void);

//Return the calibration
int get_calibration(void);
