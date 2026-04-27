// a file for creating linked lists of sentences
// Trevor Sribar
// 4/18/2026

// inlcudes
#include "sentenceLL.h"

// variables
static uint16_t numSentencesToEncrypt = 0;
static uint16_t numSentencesToSend = 0;

// 
// functions
//

// initializes the linked list, with a head for each adding, encryting, and sending
void sentenceLL_init(sentenceLinkedList_t **addHead, sentenceLinkedList_t **encryptHead, sentenceLinkedList_t **sendHead){
    *addHead = malloc(sizeof(sentenceLinkedList_t));
    *encryptHead = *addHead;
    *sendHead = *addHead;
}

// adds an element to the linked list and iterates the head to that element, adds to how many sentences must be encrypted
uint8_t sentenceLL_addSentence(sentenceLinkedList_t **head, char *sentence, uint8_t length){
    #if (SENTENCELL_DEBUG_PRINTS == 1)
    printf("add sentence: s");
    for(uint8_t i = 0; i < length; i++){
        printf("%c",sentence[i]);
    }
    printf("\n\r");
    #endif
    sentenceLinkedList_t *trueHead = *head;
    if(length>SENTENCELL_SENTENCE_SIZE){
        perror("Length is to big to add to the linked list!\n\r");
        return SENTENCELL_ERROR;
    }
    // populating current head
    memcpy(trueHead->sentence, sentence, length);
    trueHead->numCharacters=length;

    // moving to next head
    sentenceLinkedList_t *newHead = malloc(sizeof(sentenceLinkedList_t));
    if(newHead==NULL){
        perror("Malloc error: no memory!\n\r");
        return SENTENCELL_ERROR;
    }
    trueHead->next=newHead;
    *head=trueHead->next;

    // iterating the number of sentences we need to encrypt
    numSentencesToEncrypt++;

    return 0;
}

// removes the element from the linked list at the passed tail, removes how many sentences must be sent
uint8_t sentenceLL_removeSentence(sentenceLinkedList_t **tail){
    #if (SENTENCELL_DEBUG_PRINTS == 1)
    printf("remove sentence");
    printf("\n\r");
    #endif
    sentenceLinkedList_t *trueTail = *tail;
    if(numSentencesToSend==0){
        perror("No sentences to send!\n\r");
        return SENTENCELL_ERROR;
    }
    if(trueTail->next==NULL){
        perror("bad call, no valid next entry to iterate to for this linked list\n\r");
        return SENTENCELL_ERROR;
    }

    // save where our new tail is going to be
    sentenceLinkedList_t *newTail = trueTail->next;
    // free the current memory
    free(trueTail);
    // go to new tail
    *tail=newTail;

    // remove one from the number of sentences we have to send
    numSentencesToSend--;

    return 0;
}

// DOESN'T ENCRYPT, increments the head for the encryption, subtracts from how many sentences has to been encrypted, adds to how many sentences must be sent
uint8_t sentenceLL_encryptedSentence(sentenceLinkedList_t **encryption_head, char *nonce){
    #if (SENTENCELL_DEBUG_PRINTS == 1)
    printf("encrypt sentence");
    printf("\n\r");
    #endif
    sentenceLinkedList_t *trueEncryptionHead = *encryption_head;
    if(numSentencesToEncrypt==0){
        perror("No sentences to encrypt!\n\r");
        return SENTENCELL_ERROR;
    }
    if(trueEncryptionHead->next==NULL){
        perror("bad call, no valid next entry to iterate to for this linked list\n\r");
        return SENTENCELL_ERROR;
    }
    memcpy(trueEncryptionHead->sentenceNonce, nonce, ENCRYPTION_NONCE_LENGTH);
    // iterate the head
    *encryption_head=trueEncryptionHead->next;
    // remove one from the number of sentences we have to encrypt and add one to the number senteces we have to send
    numSentencesToEncrypt--;
    numSentencesToSend++;
    return 0;
}

// getter for the number of senteces we have to encrypt
uint16_t sentenceLL_getNumSentencesToEncrypt(){
    return numSentencesToEncrypt;
}

// getter for the number of senteces we have to send
uint16_t sentenceLL_getNumSentencesToSend(){
    return numSentencesToSend;
}

// frees all memory related to the linked list
void sentenceLL_destroy(sentenceLinkedList_t **sendHead){
    while(*sendHead!=NULL){
        sentenceLinkedList_t *temp = (*sendHead)->next;
        free(*sendHead);
        *sendHead = temp;
    }
}

void sentenceLL_getSentence(sentenceLinkedList_t **tail, char *sentence, uint8_t *length){
    sentenceLinkedList_t *trueTail = *tail;

    memcpy(sentence, trueTail->sentence, trueTail->numCharacters);
    *length = trueTail->numCharacters;
}

void sentenceLL_setNonce(sentenceLinkedList_t **encryption_head, char *nonce){
    memcpy((*encryption_head)->sentenceNonce, nonce, ENCRYPTION_NONCE_LENGTH);
}