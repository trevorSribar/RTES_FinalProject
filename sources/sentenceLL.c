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

// adds an element to the linked list and iterates the head to that element, adds to how many sentences must be encrypted
uint8_t sentenceLL_addSentence(sentenceLinkedList_t *head, char *sentence, uint8_t length){
    if(length>SENTENCELL_SENTENCE_SIZE){
        perror("Length of %u is to big to add to the linked list!\n\r", length);
        return SENTENCELL_ERROR;
    }
    // populating current head
    head->sentence=sentence;
    head->numCharacters=length;

    // moving to next head
    sentenceLinkedList_t *newHead = malloc(sizeof(sentenceLinkedList_t));
    head->next=newHead;
    head=head->next;

    // iterating the number of sentences we need to encrypt
    numSentencesToEncrypt++;

    return 0;
}

// removes the element from the linked list at the passed tail, removes how many sentences must be sent
uint8_t sentenceLL_removeSentence(sentenceLinkedList_t *tail){
    if(numSentencesToSend==0){
        perror("No sentences to send!\n\r");
        return SENTENCELL_ERROR;
    }
    if(tail->next==NULL){
        perror("bad call, no valid next entry to iterate to for this linked list\n\r");
        return SENTENCELL_ERROR;
    }

    // save where our new tail is going to be
    sentenceLinkedList_t *newTail = tail->next;
    // free the current memory
    free(tail);
    // go to new tail
    tail=newTail;

    // remove one from the number of sentences we have to send
    numSentencesToSend--;

    return 0;
}

// DOESN'T ENCRYPT, increments the head for the encryption, subtracts from how many sentences has to been encrypted, adds to how many sentences must be sent
uint8_t sentenceLL_encryptedSentence(sentenceLinkedList_t *encryption_head){
    if(numSentencesToEncrypt==0){
        perror("No sentences to encrypt!\n\r");
        return SENTENCELL_ERROR;
    }
    if(encryption_head->next==NULL){
        perror("bad call, no valid next entry to iterate to for this linked list\n\r");
        return SENTENCELL_ERROR;
    }
    // iterate the head
    encryption_head=encryption_head->next;
    // remove one from the number of sentences we have to encrypt and add one to the number senteces we have to send
    numSentencesToEncrypt--;
    numSentencesToSend++;
}

// getter for the number of senteces we have to encrypt
uint16_t sentenceLL_getNumSentencesToEncrypt(){
    return numSentencesToEncrypt;
}

// getter for the number of senteces we have to send
uint16_t sentenceLL_getNumSentencesToSend(){
    return numSentencesToSend;
}
