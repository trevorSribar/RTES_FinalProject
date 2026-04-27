// Toggles the laser with the GPIO pin 5 and allows the state to be read with the GPIO pin 17
// Kenneth Alcineus
// 4/23/2026

#include "laser.h"

int laser_state = 0;

void init_laser_send(void)
{
    pinMode(5, OUTPUT);
    pullUpDnControl(5, PUD_DOWN);
    pinMode(0, OUTPUT);
    pullUpDnControl(0, PUD_DOWN);
    pinMode(LASER_RECEIVER_INIDCATE_PIN, INPUT);
    pullUpDnControl(LASER_RECEIVER_INIDCATE_PIN, PUD_DOWN);
}

void init_laser_receive(void)
{
    pinMode(0, INPUT);
    pullUpDnControl(0, PUD_DOWN);
    pinMode(LASER_RECEIVER_INIDCATE_PIN, OUTPUT);
    pullUpDnControl(LASER_RECEIVER_INIDCATE_PIN, PUD_DOWN);
}

int get_laser_state(void)
{
    return laser_state;
}

int get_laser_state_gpio(void)
{
    return digitalRead(0);
}

void laser_on(void)
{
    laser_state = 1;
    digitalWrite(5, HIGH);
    digitalWrite(0, HIGH);
}

void laser_off(void)
{
    laser_state = 0;
    digitalWrite(5, LOW);
    digitalWrite(0, LOW);
}

void laser_capturedOn(){
    digitalWrite(LASER_RECEIVER_INIDCATE_PIN, HIGH);
}

void laser_capturedOff(){
    digitalWrite(LASER_RECEIVER_INIDCATE_PIN, LOW);
}

int get_laserCaptureState(){
    return digitalRead(LASER_RECEIVER_INIDCATE_PIN);
}
