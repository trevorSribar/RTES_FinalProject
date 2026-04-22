#ifndef TERMINAL_H
#define TERMINAL_H

#include "generic.h"

// Optional setup hook for future terminal config.
void terminal_init(sentenceLinkedList_t **linkedhead);

// Read one character from stdin. Returns 0 on success, -1 on EOF/error.
int terminal_read_char();

#endif
