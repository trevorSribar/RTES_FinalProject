// header for UART communication between the sending and receiving Raspberry Pis
// Kenneth Alcineus
// 4/18/2026

#include "generic.h"
#include <wiringSerial.h>

#define BAUD_RATE 115200
#define UART_DATA_TYPE_SENTENCE 1
#define UART_DATA_TYPE_SERVO 2
#define UART_MAX_DATA_TYPE 2
#define UART_MIN_DATA_TYPE 1

#define UART_DATA_TYPE_NONE 0 // this is for the case where we are receiving and see no pending bits to receive

//send over UART a string of some length and dataType;
void uart_send(char *string, int length, int dataType);

//receive a string over UART, reutrns the data type
uint8_t uart_receive(char *sentence, uint8_t *size);

//initialize UART through WiringPi library
int initialize_uart();

//close UART connection through WiringPi
void close_uart();

// syncing echo functions to sync processors

// must be called after running the receiver, otherwise the reveiver will not be looking for it
void echo_uartSender();

// must be called before running the sender, otherwise it will not be looking for the sender echo
void echo_uartReceiver();