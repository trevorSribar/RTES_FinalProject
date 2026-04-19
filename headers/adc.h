#include "ads1x1x.h"
#include "generic.h"
#include "soft_i2c.h"
#include <wiringPiI2C.h>

#define DEVICE_ID 0xFF //to be determined through actual testing

//Trigger a delay for I2C operations
void soft_i2c_delay(void);

//Set the SDA mode through the WiringPi I2C interface (can remain blank since WiringPi handles mode)
void soft_i2c_sda_mode(uint8_t value);

//Perform a SDA byte write through the WiringPi I2C interface
void soft_i2c_sda_write(uint8_t value);

//Perform a SDA byte read through the WiringPi I2C interface
uint8_t soft_i2c_sda_read(void);

//Perform a SCL write through the WiringPi I2C interface (can remain blank since WiringPi handles SCL writes)
void soft_i2c_scl_write(uint8_t value);

//Perform the I2C start sequence and write (can remain blank since WiringPi handles write starts)
uint8_t ADS1x1x_i2c_start_write(uint8_t i2c_address);

//Perform an I2C write
uint8_t ADS1x1x_i2c_write(uint8_t x);

//Perform the I2C start sequence and read (can remain blank since WiringPi handles read starts)
uint8_t ADS1x1x_i2c_start_read(uint8_t i2c_address, uint16_t bytes_to_read);

//Perform an I2C read
uint8_t ADS1x1x_i2c_read(void);

//Perform the I2C stop sequence (can remain blank since WiringPi handles stops)
uint8_t ADS1x1x_i2c_stop(void);

//Initialize the ADS1115 device
void init_ads1115(void);

//Calibrate the ADS1115 device to environment
void calibrate_ads1115(void);

//Capture <b_len> ADS1115 inputs and return a buffer containing them
uint16_t *capture_ads1115(int b_len);