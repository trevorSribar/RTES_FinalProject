#include "servo.h"

static const uint32_t position_arr[NUM_POSITIONS] = {SERVO_ANGLE_0, SERVO_ANGLE_45, SERVO_ANGLE_90, SERVO_ANGLE_135};

void servo_init(void){
    pinMode(SERVO_PIN, PWM_OUTPUT);
    pwmSetMode(PWM_MODE_MS);
    pwmSetClock(PWM_CLK_DIV); 
    pwmSetRange(1000); 

}

void servo_set_angle(uint8_t position){
    pwmWrite(SERVO_PIN, position_arr[position]);
}

// sets the servo to a random angle from the position array
uint8_t servo_set_angle_random(){
    uint8_t randomPos = rand()%NUM_POSITIONS;
    pwmWrite(SERVO_PIN, position_arr[randomPos]);
    return randomPos;
}