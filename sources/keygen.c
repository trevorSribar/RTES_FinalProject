// functions and varaibles for creating the key
// Trevor Sribar

//includes
#include "keygen.h"

// variables
static char key[ENCRYPTION_KEY_LENGTH] = {0};

//
// functions
//

// sets the inital key (0)
void keygen_init(){
    encryption_setKey(key);
}

// generates a key based on what servo data was sent and what basises were received
void keygen_sender(uint8_t *sentServoData, uint8_t *measuredServoBasis){
    // reset the key
    memset(key, 0, sizeof(key));
    // iterate over all of the key
    for(uint8_t i = 0; i < ENCRYPTION_KEY_LENGTH; i++){

        // iterate over all the bytes of data sent
        for(uint8_t j = 0; j < 8; j++){
            uint8_t sentValue = sentServoData[i*8+j];
            uint8_t measuredBasis = measuredServoBasis[i*8+j];
            // if we sent a 1 and measured in the correct basis, the key at that values should be 1, otherwise it is 0
            if(sentValue==KEYGEN_ONE_B1&&measuredBasis==KEYGEN_BASIS1)      {key[i]|=1<<j;}
            else if(sentValue==KEYGEN_ONE_B2&&measuredBasis==KEYGEN_BASIS2) {key[i]|=1<<j;}
        }
    }
    // update the key
    encryption_setKey(key);
}

// generates a key based on what data was sensed, what bases were used, and what bases were sent
void keygen_receiver(uint8_t *dataSensed, uint8_t *measuredServoData, uint8_t *reveivedServoData){
    // reset the key
    memset(key, 0, sizeof(key));
    // iterate over all of the key
    for(uint8_t i = 0; i < ENCRYPTION_KEY_LENGTH; i++){
        // iterate over all the bytes of data sent
        for(uint8_t j = 0; j < 8; j++){
            uint8_t sensedValue = dataSensed[i*8+j];
            uint8_t measuredValue = measuredServoData[i*8+j];
            uint8_t receivedBasis = reveivedServoData[i*8+j];
            // if it was sent in basis 1, we were looking at zero in that base, and we saw no value, then 1 must have been sent
            if(receivedBasis==KEYGEN_BASIS1 && measuredValue==KEYGEN_ZERO_B1 && sensedValue==0)     {key[i]|=1<<j;}
            // if it was sent in basis 1, we were looking at one in that base, and saw a value, then 1 was sent
            else if(receivedBasis==KEYGEN_BASIS1 && measuredValue==KEYGEN_ONE_B1 && sensedValue!=0) {key[i]|=1<<j;}
            // if it was sent in basis 2, we were looking at zero in that base, and we saw no value, then 1 must have been sent
            else if(receivedBasis==KEYGEN_BASIS2 && measuredValue==KEYGEN_ZERO_B2 && sensedValue==0){key[i]|=1<<j;}
            // if it was sent in basis 2, we were looking at one in that base, and saw a value, then 1 was sent
            else if(receivedBasis==KEYGEN_BASIS2 && measuredValue==KEYGEN_ONE_B2 && sensedValue!=0) {key[i]|=1<<j;}
        }
    }
    // update the key
    encryption_setKey(key);
}

// goes through the process of generating a key starting at some address and ending at another address
void keygen_senderByByte(uint8_t *sentServoData, uint8_t *measuredServoBasis, uint8_t startAddress, uint8_t endAddress){
    // if we are starting at nothing, reset the key
    if(startAddress==0){
        memset(key, 0, sizeof(key));
    }
    // iterate over all of the key
    for(uint8_t i = startAddress; i <= endAddress; i++){

        // iterate over all the bytes of data sent
        for(uint8_t j = 0; j < 8; j++){
            uint8_t sentValue = sentServoData[i*8+j];
            uint8_t measuredBasis = measuredServoBasis[i*8+j];
            // if we sent a 1 and measured in the correct basis, the key at that values should be 1, otherwise it is 0
            if(sentValue==KEYGEN_ONE_B1&&measuredBasis==KEYGEN_BASIS1)      {key[i]|=1<<j;}
            else if(sentValue==KEYGEN_ONE_B2&&measuredBasis==KEYGEN_BASIS2) {key[i]|=1<<j;}
        }
    }
    // if we are done doing all addresses, update the key
    if(endAddress+1==ENCRYPTION_KEY_LENGTH){
        encryption_setKey(key);
    }
}

// goes through the process of generating a key starting ast some address and ending at another address
void keygen_receiverByByte(uint8_t *dataSensed, uint8_t *measuredServoData, uint8_t *reveivedServoData, uint8_t startAddress, uint8_t endAddress){
    // if we are starting at nothing, reset the key
    if(startAddress==0){
        memset(key, 0, sizeof(key));
    }
    // iterate over all of the key
    for(uint8_t i = startAddress; i <= endAddress; i++){
        // iterate over all the bytes of data sent
        for(uint8_t j = 0; j < 8; j++){
            uint8_t sensedValue = dataSensed[i*8+j];
            uint8_t measuredValue = measuredServoData[i*8+j];
            uint8_t receivedBasis = reveivedServoData[i*8+j];
            // if it was sent in basis 1, we were looking at zero in that base, and we saw no value, then 1 must have been sent
            if(receivedBasis==KEYGEN_BASIS1 && measuredValue==KEYGEN_ZERO_B1 && sensedValue==0)     {key[i]|=1<<j;}
            // if it was sent in basis 1, we were looking at one in that base, and saw a value, then 1 was sent
            else if(receivedBasis==KEYGEN_BASIS1 && measuredValue==KEYGEN_ONE_B1 && sensedValue!=0) {key[i]|=1<<j;}
            // if it was sent in basis 2, we were looking at zero in that base, and we saw no value, then 1 must have been sent
            else if(receivedBasis==KEYGEN_BASIS2 && measuredValue==KEYGEN_ZERO_B2 && sensedValue==0){key[i]|=1<<j;}
            // if it was sent in basis 2, we were looking at one in that base, and saw a value, then 1 was sent
            else if(receivedBasis==KEYGEN_BASIS2 && measuredValue==KEYGEN_ONE_B2 && sensedValue!=0) {key[i]|=1<<j;}
        }
    }
    // if we are done doing all addresses, update the key
    if(endAddress+1==ENCRYPTION_KEY_LENGTH){
        encryption_setKey(key);
    }
}