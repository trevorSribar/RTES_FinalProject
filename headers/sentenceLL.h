// a file for creating linked lists of sentences
// Trevor Sribar
// 4/18/2026

// includes
#include "generic.h"

// defines
#define SENTENCELL_SENTENCE_SIZE 100 // max number of characters in a sentence
#define SENTENCELL_ERROR 1

// structs
typedef struct sentenceLinkedList {
    char sentence[SENTENCELL_SENTENCE_SIZE];
    uint8_t numCharacters;
    struct sentenceLinkedList *next;
} sentenceLinkedList_t;

//
// funtions
//

// initializes the linked list, with a head for each adding, encryting, and sending
void sentenceLL_init(sentenceLinkedList_t *addHead, sentenceLinkedList_t *encryptHead, sentenceLinkedList_t *sendHead);

// adds an element to the linked list and iterates the head to that element, adds to how many sentences must be encrypted
uint8_t sentenceLL_addSentence(sentenceLinkedList_t *head, char *sentence, uint8_t length);

// removes the element from the linked list at the passed tail, removes how many sentences must be sent
uint8_t sentenceLL_removeSentence(sentenceLinkedList_t *tail);

// DOESN'T ENCRYPT, increments the head for the encryption, subtracts from how many sentences has to been encrypted, adds to how many sentences must be sent
uint8_t sentenceLL_encryptedSentence(sentenceLinkedList_t *encryption_head);

// getter for the number of senteces we have to encrypt
uint16_t sentenceLL_getNumSentencesToEncrypt();

// getter for the number of senteces we have to send
uint16_t sentenceLL_getNumSentencesToSend();