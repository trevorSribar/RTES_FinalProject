// a file for enumerating the encrypting using chacha20
// Trevor Sribar
// 4/18/2026

// includes
#include <mbedtls/chacha20.h>
#include "generic.h"

// defines
#define ENCRYPTION_ERROR 1
#define ENCRYPTION_KEY_LENGTH 32
#define ENCRYPTION_NONCE_LENGTH 12

//
// functions
//

// initializes encryption variables
uint8_t encryption_init();

// cleans the memory that was used for encryption
void encryption_destroy();

// generates a new nonce, a 12 byte string
void encryption_updateNonce();

// returns the nonce address
unsigned char *encryption_getNonceAddress();

// sets the key array to the passed key;
void encryption_setKey(char *newKey);

// gets the key array
unsigned char *encryption_getKey();

// encrypts input character string, the character string will be replaced with the encrypted data
uint8_t encryption_encryptData(char *input, uint16_t inputSize);