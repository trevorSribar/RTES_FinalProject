ADS1x1x_config_t ads1115_cfg;
int fd;
uint16_t calibration = 0;

void init_ads1115(void)
{
    // Initialise ADC
    if (ADS1x1x_init(&ads1115_cfg, ADS1115, DEVICE_ID, MUX_SINGLE_0, PGA_4096) == 0)
    {
        printf("Could not initialize ADS1115 - Module and photoresistor cannot be read\n");
    }

    ADS1x1x_set_data_rate(&ads1115_cfg, DATA_RATE_ADS111x_860);
}

void calibrate_ads1115(void)
{
    calibration = ADS1x1x_read(&ads1115_cfg);
}

void capture_ads1115(uint16_t *buffer, int b_len)
{
    for (int i = 0; i < b_len; i++)
    {
        buffer[i] = ADS1x1x_read(&ads1115_cfg) - calibration;
    }
}
