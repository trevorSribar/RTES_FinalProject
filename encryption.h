// a file for enumerating the encrypting using chacha20
// Trevor Sribar
// 4/18/2026

// includes
#include <mbedtls/chacha20.h>

// defines
#define ENCRYPT_ERROR 1

//
// functions
//

// initializes encryption variables
uint8_t encryption_init();

// generates a new nonce, a 12 byte string
void encryption_updateNonce();

void encryption_getNonceAddress();

// encrypts input character string, the character string will be replaced with the encrypted data
uint8_t encryption_encryptData(char *input, uint16_t inputSize);