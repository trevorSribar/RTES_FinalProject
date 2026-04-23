#include "encryption.h"

static unsigned char dummyKey[ENCRYPTION_KEY_LENGTH] = {0};
static unsigned char dummyNonce[ENCRYPTION_NONCE_LENGTH] = {0};

uint8_t encryption_init()
{
    return 0;
}

void encryption_destroy()
{
}

void encryption_updateNonce()
{
}

unsigned char *encryption_getNonceAddress()
{
    return dummyNonce;
}

void encryption_setKey(char *newKey)
{
    if (newKey == NULL)
    {
        return;
    }

    for (uint8_t i = 0; i < ENCRYPTION_KEY_LENGTH; i++)
    {
        dummyKey[i] = (unsigned char)newKey[i];
    }
}

unsigned char *encryption_getKey()
{
    return dummyKey;
}

uint8_t encryption_encryptData(char *input, uint16_t inputSize)
{
    (void)input;
    (void)inputSize;
    return 0;
}

uint8_t encryption_decryptData(char *input, uint16_t inputSize, unsigned char *currentNonce)
{
    (void)input;
    (void)inputSize;
    (void)currentNonce;
    return 0;
}
