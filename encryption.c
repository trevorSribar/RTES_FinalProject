// functions and varaibles for encryptiong is chacha20
// Trevor Sribar

// includes
#include "encryption.h"

// variables
static mbedtls_chacha20_context ctx;
static unsigned char key[ENCRYPTION_KEY_LENGTH];
static unsigned char nonce[ENCRYPTION_NONCE_LENGTH];
static uint32_t counter = 0;

//
// functions
//

// encryption
uint8_t encryption_init(){
    mbedtls_chacha20_init(&ctx); //init
    if(mbedtls_chacha20_setkey(&ctx, key)!=0){ //set key
        perror("Key gen failed\n\r");
        return ENCRYPTION_ERROR;
    }
    if(mbedtls_chacha20_starts(&ctx, nonce, counter)){
        perror("Nonce gen failed\n\r");
        return ENCRYPTION_ERROR;
    }
    return 0;
}

// cleans the memory that was used for encryption
void encryption_destroy(){
    mbedtls_chacha20_free(&ctx);
}

// generates a new nonce
void encryption_updateNonce(){
    for(uint8_t i = 0; i < ENCRYPTION_NONCE_LENGTH; i++){
        nonce[i] = rand();
    }
}

// returns the nonce address
unsigned char *encryption_getNonceAddress(){
    return nonce;
}

// sets the key array to the passed key;
void encryption_setKey(char *newKey){
    for(uint8_t i = 0; i < ENCRYPTION_KEY_LENGTH; i++){
        key[i] = newKey[i];
    }
}

// gets the key array
unsigned char *encryption_getKey(){
    return key;
}

// encrypts input character string, the character string will be replaced with the encrypted data
uint8_t encryption_encryptData(char *input, uint16_t inputSize){
    // storing the output data
    char output[inputSize];

    // encryting the data
    if(mbedtls_chacha20_crypt(key, nonce, counter, inputSize, input, output)!=0){
        perror("Encryption Error\n\r");
        return ENCRYPTION_ERROR;
    }
    // setting updating the input to be the output
    for(uint16_t i = 0; i < inputSize; i++){
        input[i] = output[i];
    }
    return 0;
}

