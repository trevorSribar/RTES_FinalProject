#include "uart.h"

int programFlag = 1;
int serialPort;
char buffer[BUFFER_SIZE + 2];

void uart_send(char *s, int s_len, int s_type)
{
  if (s_len > BUFFER_SIZE)
  {
    s_len = BUFFER_SIZE;
  }

  serialPutchar(serialPort, (0x11 + s_type)); //s_type should only be of values 0, 1, or 2 to take advantage of device control characters

  for (int i = 0; i < s_len; i++)
  {
    //may need to revisit to make this real-time friendly to account so that services can run between individual character sends
    serialPutchar(serialPort, s[i]);
  }
  
  serialPutchar(serialPort, 0x14); // DC4 will signify string termination in place of NUL
}

char *uart_receive()
{
  memset(buffer, '\0', sizeof(buffer));

  if (!serialDataAvail(serialPort))
  {
    return buffer;
  }

  buffer[0] = serialGetchar(serialPort);

  switch (buffer[0])
  {
    case 0x11: //unencrypted string
      break;
    case 0x12: //encrypted string
      break;
    case 0x13: //laser toggle signal
      break;
    default: //in any other situation don't receive string
      return buffer;
  }

  for (int i = 1; i < BUFFER_SIZE + 1; i++)
  {
    //may need to revisit to make this real-time friendly. for now, adjust polls in uart.h
    int data_polls = 0;
    while (!serialDataAvail(serialPort))
    {
      data_polls++;
      if (data_polls >= MAX_POLLS)
      {
        return buffer;
      }
    }
    
    buffer[i] = serialGetchar(serialPort);

    if (buffer[i] == 0x14)
    {
      return buffer;
    }
  }

  if (buffer[BUFFER_SIZE + 1] != 0x14) //possible error condition: end of buffer reached but DC4 termination character not found
  {
    return NULL;
  }

  return buffer;
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

  if (wiringPiSetup() == -1)
  {
    printf("Could not initialize WiringPi library\n");
    syslog(LOG_PERROR, "Could not initialize WiringPi library\n");
    return 1;
  }

  return 0;
}

void close_uart()
{
  programFlag = 0;
  serialClose(serialPort);
}

int main()
{
  if (TEST_SET)
  {
    if (initialize_uart())
    {
      return 1;
    }

    if (TEST_SET == 1)
    {
      sender_test_set();
    }

    else if (TEST_SET == 2)
    {
      receiver_test_set();
    }

    close_uart();
  }

  else
  {
    while(programFlag) {} //loop until close_uart() is called for UART to be used elsewhere
  }

  return 0;
}
