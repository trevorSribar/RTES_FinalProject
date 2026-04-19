#include "generic.h"
#include <syslog.h>
#include <wiringSerial.h>

#define BAUD_RATE 115200
#define BUFFER_SIZE 100
#define MAX_POLLS 500
#define TEST_SET 1

//send string s over UART over a length of s_len with a type of s_type representing unencrypted as 0, encrypted as 1, and laser toggle as 2
void uart_send(char *s, int s_len, int s_type);

//receive a string over UART
char *uart_receive();

//test set function for the sending Raspberry Pi (TEST_SET=1)
void sender_test_set();

//test set function for the receiving Raspberry Pi (TEST_SET=2)
void receiver_test_set();

//initialize UART through WiringPi library
int initialize_uart();

//close UART connection through WiringPi
void close_uart();
