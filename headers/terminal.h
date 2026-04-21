#ifndef TERMINAL_H
#define TERMINAL_H

#include "generic.h"

#define TERMINAL_INPUT_MAX 128

// Optional setup hook for future terminal config.
void terminal_init(void);


// Read one character from stdin. Returns 0 on success, -1 on EOF/error.
int terminal_read_char();



#endif
