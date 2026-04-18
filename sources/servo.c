#include "servo.h"

static const uint8_t position_arr[NUM_POSITIONS] = {SERVO_ANGLE_0, SERVO_ANGLE_45, SERVO_ANGLE_90, SERVO_ANGLE_135};

void servo_init(void){
    if (wiringPiSetup() == -1) {
        // Handle error
    }

    pinMode(SERVO_PIN, PWM_OUTPUT);
    pwmSetMode(PWM_MODE_MS);
    pwmSetClock(PWM_CLK_DIV);
    pwmSetRange(1000); 

}

void servo_set_angle(uint8_t position){
    pwmWrite(SERVO_PIN, position_arr[position]);
}

// sets the servo to a random position, and returns the position it was set to
uint8_t servo_set_angle_random(void){
    uint8_t position = rand()%NUM_POSITIONS;
    servo_set_angle(position);
    return position;
}
