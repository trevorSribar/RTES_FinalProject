#include "laser.h"

int laser_state = 0;

void init_laser(void)
{
    wiringPiSetupGpio();
    pinMode(5, OUTPUT);
    pullUpDnControl(5, PUD_DOWN);
    if (PI_TYPE == 1)
    {
        pinMode(17, OUTPUT);
    }
    else
    {
        pinMode(17, INPUT);
    }
    pullUpDnControl(17, PUD_DOWN);
}

int get_laser_state(void)
{
    return laser_state;
}

void laser_on(void)
{
    laser_state = 1;
    digitalWrite(5, HIGH);
    if (PI_TYPE == 1)
    {
        digitalWrite(17, HIGH);
    }
}

void laser_off(void)
{
    laser_state = 0;
    digitalWrite(5, LOW);
    if (PI_TYPE == 1)
    {
        digitalWrite(17, LOW);
    }
}
