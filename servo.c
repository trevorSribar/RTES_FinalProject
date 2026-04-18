#include "servo.h"

static const int position_arr[NUM_POSITIONS] = {SERVO_ANGLE_0, SERVO_ANGLE_45, SERVO_ANGLE_90, SERVO_ANGLE_135};

void servo_init(void){
    if (wiringPiSetup() == -1) {
        // Handle error
    }

    pinMode(SERVO_PIN, PWM_OUTPUT);
    pwmSetMode(PWM_MODE_MS);
    pwmSetClock(PWM_CLK_DIV);
    pwmSetRange(1000); 

}

void servo_set_angle(int position){
    pwmWrite(SERVO_PIN, position_arr[position]);
}