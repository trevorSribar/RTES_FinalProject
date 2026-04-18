
#include <WiringPi.h>

#define SERVO_PIN 1 
#define PWM_CLOCK 19200U * 1000U
#define PWM_CLK_DIV 192U
#define PWM_RANGE 2000U
#define PWM_FREQUENCY ((PWM_CLOCK / PWM_CLK_DIV) / PWM_RANGE)

#define NUM_POSITIONS 4
#define SERVO_ANGLE_0 100
#define SERVO_ANGLE_45 125
#define SERVO_ANGLE_90 150
#define SERVO_ANGLE_135 175

//initializes the servo motor pwm output
void servo_init(void);

// sets the servo angle to the specified position (0, 45, 90, or 135 degrees)
void servo_set_angle(int angle);