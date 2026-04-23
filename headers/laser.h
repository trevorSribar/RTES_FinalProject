// Header to expose the laser toggle and state read functions
// Kenneth Alcineus
// 4/23/2026

#include "generic.h"

#define LASER_TIME_ON 3500

void init_laser_send(void);

void init_laser_receive(void);

int get_laser_state(void);

int get_laser_state_gpio(void);

void laser_on(void);

void laser_off(void);
