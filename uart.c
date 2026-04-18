#include "uart.h"

int serialPort;

void uart_send(char *s, int s_len)
{
  char tempbuffer[s_len];
  strcpy(tempbuffer, s, sizeof(tempbuffer));

  for (int i = 0; i < s_len; i++)
  {
    serialPutchar(serialPort, tempbuffer[i]);
  }
}

char *uart_receive()
{
  char buffer[BUFFER_SIZE];
  memset(buffer, '\0', sizeof(buffer));

  for (int i = 0; i < BUFFER_SIZE; i++)
  {
    if (!serialDataAvail(serialPort))
    {
      break;
    }
    buffer[i] = serialGetchar(serialPort);
  }

  return buffer;
}

void sender_test_set()
{
  char sbuffer[BUFFER_SIZE];
  memset(sbuffer, '\0', sizeof(sbuffer));

  uart_send("foo", 3);

  printf("Sent string: foo\n");

  printf("Waiting for string...\n");

  while (1)
  {
    sbuffer = uart_receive();
    if (sbuffer[0] != '\0')
    {
      break;
    }
  }

  printf("Received string: %s\n", sbuffer);

  memset(sbuffer, '\0', sizeof(sbuffer));
}

void receiver_test_set()
{
  char rbuffer[BUFFER_SIZE];
  memset(rbuffer, '\0', sizeof(rbuffer));
  
  printf("Waiting for string...\n");

  while (1)
  {
    rbuffer = uart_receive();
    if (rbuffer[0] != '\0')
    {
      break;
    }
  }

  printf("Received string: %s\n", rbuffer);

  memset(sbuffer, '\0', sizeof(sbuffer));

  uart_send("bar", 3);

  printf("Sent string: bar\n");
}

int main()
{
  serialPort = serialOpen("/dev/ttyS0", BAUD_RATE);

  if (serialPort < 0)
  {
    syslog(LOG_PERROR, "Could not open serial device\n");
    return 1;
  }

  if (wiringPiSetup() == -1)
  {
    syslog(LOG_PERROR, "Could not initialize WiringPi library\n");
    return 1;
  }

#ifdef TEST_SET
  sender_test_set();
#else
  receiver_test_set();
#endif

  serialClose(serialPort);
}