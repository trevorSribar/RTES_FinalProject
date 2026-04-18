#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <WiringPi.h>
#include <WiringSerial.h>

#define BAUD_RATE 115200
#define BUFFER_SIZE 100
#define TEST_SET 1

//send string <s> over UART over a length of <s_len>
void uart_send(char *s, int s_len);

//receive a string over UART
char *uart_receive();

//test set function for the sending Raspberry Pi (TEST_SET=1)
void sender_test_set();

//test set function for the receiving Raspberry Pi (TEST_SET=0)
void receiver_test_set();