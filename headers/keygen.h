// a file for creating the key based on the servo input
// Trevor Sribar
// 4/18/2026

// includes
#include "encryption.h"

// defines
#define KEYGEN_ZERO_B1 0
#define KEYGEN_ZERO_B2 1
#define KEYGEN_ONE_B1 2
#define KEYGEN_ONE_B2 3
#define KEYGEN_BASIS1 0
#define KEYGEN_BASIS2 1

//
// functions
//

// generates a key based on what servo data was sent and what basises were received
void keygen_sender(uint8_t *sentServoData, uint8_t *measuredServoBasis);

// generates a key based on what data was sensed, what bases were used, and what bases were sent
void keygen_receiver(uint8_t *dataSensed, uint8_t *measuredServoData, uint8_t *reveivedServoData);