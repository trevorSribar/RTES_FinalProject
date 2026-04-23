// header for UART communication between the sending and receiving Raspberry Pis
// Kenneth Alcineus
// 4/18/2026

#include "generic.h"
#include <wiringSerial.h>

#define BAUD_RATE 115200
#define BUFFER_SIZE 256
#define TEST_SET 1
#define UART_SENDER_SERVO_DATA 0x13
#define UART_SENDER_SENTENCE_ENCRYPTED 0x12 
#define UART_SENDER_SENTENCE_UNENCRYPTED 0x11 
#define UART_NUM_CHECK_EMPTY_BE_SURE 3

//send string s over UART over a length of s_len with a type of s_type representing unencrypted as 0, encrypted as 1, and laser toggle as 2
void uart_send(char *s, int s_len, int s_type);

//receive a string over UART
char *uart_receive();

//return the length of the string that has not yet been fully processed by the receiver
int uart_str_len();

//test set function for the sending Raspberry Pi (TEST_SET=1)
void sender_test_set();

//test set function for the receiving Raspberry Pi (TEST_SET=2)
void receiver_test_set();

//initialize UART through WiringPi library
int initialize_uart();

//close UART connection through WiringPi
void close_uart();
