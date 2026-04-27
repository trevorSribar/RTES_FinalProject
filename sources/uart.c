// Enables bidirectional UART communication between the sending and receiving Raspberry Pis
// Kenneth Alcineus
// 4/18/2026

// Modified by Trevor Sribar
// Modified uart_send, uart_receive, removed 3 usless functions
// 4/26/2026

#include "uart.h"

int serialPort;

//send over UART a string of some length and dataType;
void uart_send(char *string, int length, int dataType)
{
  if (length > 256){ // if length is bigger than a char
    perror("UART Error: Invalid data size\n");
    return;
  }
  if (dataType > UART_MAX_DATA_TYPE || dataType < UART_MIN_DATA_TYPE)
  {
    perror("UART Write Error: Invalid data type\n");
    return;
  }

  serialPutchar(serialPort, dataType);
  serialPutchar(serialPort, length);

  for (int i = 0; i < length; i++)
  {
    serialPutchar(serialPort, string[i]);
  }
}

// receive a string over UART, reutrns the data type
uint8_t uart_receive(char *sentence, uint8_t *size)
{
  if (!serialDataAvail(serialPort))
  {
    return 0;
  }

  uint8_t dataType = serialGetchar(serialPort); // the first data MUST be the data type
  if (dataType > UART_MAX_DATA_TYPE || dataType < UART_MIN_DATA_TYPE)
  {
    perror("UART Read Error: Invalid data type\n");
    serialFlush(serialPort);
    return -1;
  }

  while (!serialDataAvail(serialPort)); // wait for more data to come

  // read size byte into uint16_t first so we can handle the 0=256 case without losing range
  uint16_t sizeToLoopOver = (uint8_t)serialGetchar(serialPort); // 0 to 255, where 0 means 256
  if(size != NULL){
    *size = (uint8_t)sizeToLoopOver;
  }
  if(sizeToLoopOver==0){
    sizeToLoopOver=256;
  }

  for (uint16_t i = 0; i < sizeToLoopOver; i++){
    while (!serialDataAvail(serialPort)) {}
    sentence[i] = serialGetchar(serialPort);
  }

  return dataType;
}

// must be called after running the receiver, otherwise the reveiver will not be looking for it
void echo_uartReceiver(){
  // send the wait for the echo
  serialPutchar(serialPort, 0xFF);
  while(!serialDataAvail(serialPort));

  // pull the FF
  serialGetchar(serialPort);
}

// must be called before running the sender, otherwise it will not be looking for the sender echo
void echo_uartSender(){
  // wait then send the echo
  while(!serialDataAvail(serialPort));
  serialPutchar(serialPort, 0xFF);

  // pull the FF
  serialGetchar(serialPort);
}

// initializes UART
int initialize_uart()
{
  serialPort = serialOpen("/dev/ttyS0", BAUD_RATE);

  if (serialPort < 0)
  {
    perror("UART init: Could not open serial device\n");
    syslog(LOG_PERROR, "Could not open serial device\n");
    return 1;
  }

  return 0;
}

void close_uart()
{
  serialClose(serialPort);
}
