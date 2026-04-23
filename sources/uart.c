// Enables bidirectional UART communication between the sending and receiving Raspberry Pis
// Kenneth Alcineus
// 4/18/2026

#include "uart.h"

int programFlag = 1;
int serialPort;
char buffer[BUFFER_SIZE + 2];

void uart_send(char *s, int s_len, int s_type)
{
  if (s_len >= BUFFER_SIZE)
  {
    perror("string length is too large!");
    return;
  }

  if (s_type > 2 || s_type < 0)
  {
    return;
  }

  char dataType = 0x11 + s_type; //s_type should only be of values 0, 1, or 2 to represent unencrypted, encrypted, and sender servo data

  serialPutchar(serialPort, dataType);
  serialPutchar(serialPort, (char)s_len); //cast s_len to char, string length can never be greater than 255 anyway

  for (int i = 0; i < s_len; i++)
  {
    serialPutchar(serialPort, s[i]);
  }
}

char *uart_receive()
{
  if (!serialDataAvail(serialPort))
  {
    return NULL;
  }

  memset(buffer, '\0', sizeof(buffer));
  buffer[0] = serialGetchar(serialPort);

  // Always treat the first byte as a frame type to avoid stale-state desync.
  switch (buffer[0])
  {
    case 0x11: //unencrypted string
    case 0x12: //encrypted string
    case 0x13: //servo data
      break;
    default: //in any other situation don't receive string - perform a serial flush

      serialFlush(serialPort);
      return NULL;
  }

  while (!serialDataAvail(serialPort)) {}

  buffer[1] = serialGetchar(serialPort);
  int strLen = (int)((uint8_t)buffer[1]);

  if (strLen > BUFFER_SIZE)
  {

    serialFlush(serialPort);
    return NULL;
  }
  int dataCount = strLen;

  for (int i = 2; i < strLen + 2; i++)
  {
    while (!serialDataAvail(serialPort)) {}
    
    buffer[i] = serialGetchar(serialPort);
    dataCount--;

    if (dataCount == 0)
    {
      serialFlush(serialPort);
      break;
    }
  }

  return buffer;
}

int uart_str_len()
{
  return (int)((uint8_t)buffer[1]);
}

void sender_test_set()
{
  char sbuffer[BUFFER_SIZE];
  int sflag = 1;

  memset(sbuffer, '\0', sizeof(sbuffer));

  uart_send("foo", 3, 0);

  printf("Sent string: foo\n");

  printf("Waiting for string...\n");

  while (sflag)
  {
    if (serialDataAvail(serialPort))
    {
      sflag = 0;
    }
    strncpy(sbuffer, uart_receive(), sizeof(sbuffer));
  }

  printf("Received string: %s\n", sbuffer);

  memset(sbuffer, '\0', sizeof(sbuffer));
}

void receiver_test_set()
{
  char rbuffer[BUFFER_SIZE];
  int rflag = 1;

  memset(rbuffer, '\0', sizeof(rbuffer));
  
  printf("Waiting for string...\n");

  while (rflag)
  {
    if (serialDataAvail(serialPort))
    {
      rflag = 0;
    }
    strncpy(rbuffer, uart_receive(), sizeof(rbuffer));
  }

  printf("Received string: %s\n", rbuffer);

  memset(rbuffer, '\0', sizeof(rbuffer));

  uart_send("bar", 3, 0);

  printf("Sent string: bar\n");
}

int initialize_uart()
{
  serialPort = serialOpen("/dev/ttyS0", BAUD_RATE);

  if (serialPort < 0)
  {
    printf("Could not open serial device\n");
    syslog(LOG_PERROR, "Could not open serial device\n");
    return 1;
  }

  return 0;
}

void close_uart()
{
  programFlag = 0;
  serialClose(serialPort);
}
