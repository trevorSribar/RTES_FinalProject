// functions and varaibles for encryptiong is chacha20
// Trevor Sribar

// includes
#include "encryption.h"

// variables
static mbedtls_chacha20_context ctx;
static unsigned char key[32];
static unsigned char nonce[12];
static uint32_t counter = 0;

//
// functions
//

// encryption
uint8_t encryption_init(){
    mbedtls_chacha20_init(&ctx); //init
    for(uint8_t i = 0; i < 32; i++){ //random key
        key[i] = rand();
    }
    if(mbedtls_chacha20_setkey(&ctx, key)!=0){ //set key
        printf("Key gen failed\n\r");
        return ENCRYPT_ERROR;
    }
    if(mbedtls_chacha20_starts(&ctx, nonce, counter)){
        printf("Nonce gen failed");
        return ENCRYPT_ERROR;
    }
    return 0;
}

// generates a new nonce
void encryption_updateNonce(){
    for(uint8_t i = 0; i < 12; i++){
        nonce[i] = rand();
    }
}

// encrypts input character string, the character string will be replaced with the encrypted data
uint8_t encryption_encryptData(char *input, uint16_t inputSize){
    // storing the output data
    char output[inputSize];

    // encryting the data
    if(mbedtls_chacha20_crypt(key, nonce, counter, 80, input, output)!=0){
        return ENCRYPT_ERROR;
    }
    // setting updating the input to be the output
    for(uint16_t i = 0; i < inputSize; i++){
        input[i] = output[i];
    }
    return 0;
}

