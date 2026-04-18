// functions and varaibles for creating the key
// Trevor Sribar

//includes
#include "keygen.h"

//
// functions
//

// generates a key based on what servo data was sent and what basises were received
void keygen_sender(uint8_t *sentServoData, uint8_t *measuredServoBasis){
    // reset the key
    char key[ENCRYPTION_KEY_LENGTH] = {0};
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
    char key[ENCRYPTION_KEY_LENGTH] = {0};
    // iterate over all of the key
    for(uint8_t i = 0; i < ENCRYPTION_KEY_LENGTH; i++){
        // iterate over all the bytes of data sent
        for(uint8_t j = 0; j < 8; j++){
            uint8_t sensedValue = dataSensed[i*8+j];
            uint8_t measuredValue = measuredServoData[i*8+j];
            uint8_t receivedBasis = reveivedServoData[i*8+j];
            // if it was sent in basis 1, we were looking at zero in that base, and we saw no value
            if(receivedBasis==KEYGEN_BASIS1 && measuredValue==KEYGEN_ZERO_B1 && sensedValue==0)     {key[i]|=1<<j;}
            // if it was sent in basis 1, we were looking at one in that base, and saw a value
            else if(receivedBasis==KEYGEN_BASIS1 && measuredValue==KEYGEN_ONE_B1 && sensedValue!=0) {key[i]|=1<<j;}
            // if it was sent in basis 2, we were looking at zero in that base, and we saw no value
            else if(receivedBasis==KEYGEN_BASIS2 && measuredValue==KEYGEN_ZERO_B2 && sensedValue==0){key[i]|=1<<j;}
            // if it was sent in basis 2, we were looking at one in that base, and saw a value
            else if(receivedBasis==KEYGEN_BASIS2 && measuredValue==KEYGEN_ONE_B2 && sensedValue!=0) {key[i]|=1<<j;}
        }
    }
    // update the key
    encryption_setKey(key);
}