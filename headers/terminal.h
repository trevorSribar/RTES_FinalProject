#ifndef TERMINAL_H
#define TERMINAL_H

#include "generic.h"
#include "sentenceLL.h"


// Optional setup hook for future terminal config.
void terminal_init(sentenceLinkedList_t **linkedhead);

// Read one character from stdin. Returns 0 on success, -1 on EOF/error.
int terminal_read_char();

// Prints the decrypted sentence at the tail of the linked list, then iterates the tail to the next node.
void terminal_printDecryptedSentence(sentenceLinkedList_t **tail);
#endif
