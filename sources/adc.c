/*
Notes: may need to replace contents of ADS1x1x_write_register and ADS1x1x_read_register in ads1x1x.c 
with wiringPiI2CWriteReg16 and wiringPiI2CReadReg16 respectively. Once that is done, there may not be
any need to implement any of the extern functions at all
*/

ADS1x1x_config_t ads1115_cfg;
int fd;
uint16_t calibration = 0;

void soft_i2c_delay(void)
{
    //insert sleep function here - find shortest possible delay time for I2C that would still guarantee data transmission
}

void soft_i2c_sda_mode(uint8_t value) {}

void soft_i2c_sda_write(uint8_t value)
{
    wiringPiI2CWrite(fd, (int)value);
}

uint8_t soft_i2c_sda_read(void)
{
    return wiringPiI2CRead(fd);
}

void soft_i2c_scl_write(uint8_t value); {}

uint8_t ADS1x1x_i2c_start_write(uint8_t i2c_address) {}

uint8_t ADS1x1x_i2c_write(uint8_t x)
{
    soft_i2c_scl_write(x);
}

uint8_t ADS1x1x_i2c_start_read(uint8_t i2c_address, uint16_t bytes_to_read) {}

uint8_t ADS1x1x_i2c_read(void);
{
    return soft_i2c_sda_read();
}

uint8_t ADS1x1x_i2c_stop(void) {}

void init_ads1115(void)
{
    // Initialize I2C interface
    fd = wiringPiI2CSetup(DEVICE_ID);

    // Initialise ADC
    if (ADS1x1x_init(&ads1115_cfg, ADS1115, DEVICE_ID, MUX_SINGLE_0, PGA_4096) == 0)
    {
        printf("Could not initialize ADS1115 - Photoresistor reads cannot be read\n");
    }

    ADS1x1x_set_data_rate(&ads1115_cfg, DATA_RATE_ADS111x_860);
}

void calibrate_ads1115(void)
{
    calibration = ADS1x1x_read(&ads1115_cfg);
}

//Capture <b_len> ADS1115 inputs and return a buffer containing them
uint16_t *capture_ads1115(int b_len)
{
    uint16_t buffer[b_len];

    for (int i = 0; i < b_len; i++)
    {
        buffer[i] = ADS1x1x_read(&ads1115_cfg) - calibration;
    }

    return buffer;
}
