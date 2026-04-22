#ifndef DUMMY_WIRING_PI_H
#define DUMMY_WIRING_PI_H

#include <unistd.h>

/*
 * Dummy WiringPi shim for test builds on systems without WiringPi.
 * These definitions are no-ops and should only be used when tests do not
 * rely on real GPIO/I2C/UART behavior.
 */

#define INPUT 0
#define OUTPUT 1
#define PWM_OUTPUT 2

#define LOW 0
#define HIGH 1

#define PWM_MODE_MS 0

static inline int wiringPiSetup(void)
{
	return 0;
}

static inline void pinMode(int pin, int mode)
{
	(void)pin;
	(void)mode;
}

static inline void digitalWrite(int pin, int value)
{
	(void)pin;
	(void)value;
}

static inline int digitalRead(int pin)
{
	(void)pin;
	return LOW;
}

static inline void pwmSetMode(int mode)
{
	(void)mode;
}

static inline void pwmSetClock(int divisor)
{
	(void)divisor;
}

static inline void pwmSetRange(unsigned int range)
{
	(void)range;
}

static inline void pwmWrite(int pin, int value)
{
	(void)pin;
	(void)value;
}

static inline void delay(unsigned int howLong)
{
	usleep(howLong * 1000U);
}

static inline void delayMicroseconds(unsigned int howLong)
{
	usleep(howLong);
}

#endif
