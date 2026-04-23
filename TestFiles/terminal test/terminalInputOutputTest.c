#define _GNU_SOURCE

#include "generic.h"
#include "terminal.h"
#include "encryption.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

#define TERMINAL_POLL_PERIOD_NS 30000000L
#define TERMINAL_FLUSH_PERIOD_SEC 15L

static sentenceLinkedList_t *addHead = NULL;
static sentenceLinkedList_t *encryptHead = NULL;
static sentenceLinkedList_t *sendHead = NULL;

static pthread_mutex_t llMutex = PTHREAD_MUTEX_INITIALIZER;
static volatile sig_atomic_t stopRequested = 0;

static inline void add_ns_to_timespec(struct timespec *ts, long ns)
{
	ts->tv_nsec += ns;
	while (ts->tv_nsec >= 1000000000L)
	{
		ts->tv_sec++;
		ts->tv_nsec -= 1000000000L;
	}
}

static void flush_linked_list(void)
{
	char dummyNonce[ENCRYPTION_NONCE_LENGTH] = {0};
	while (sentenceLL_getNumSentencesToEncrypt() > 0)
	{
		(void)sentenceLL_encryptedSentence(&encryptHead, dummyNonce);
	}

	while (sentenceLL_getNumSentencesToSend() > 0)
	{
		char dummyByte = 0;
		terminal_print_and_delete_DecryptedSentence(&sendHead);
	}
}

static void *terminal_reader_thread(void *threadp)
{
	(void)threadp;

	struct timespec nextRelease;
	clock_gettime(CLOCK_MONOTONIC, &nextRelease);

	while (!stopRequested)
	{
		pthread_mutex_lock(&llMutex);
		(void)terminal_read_char();
		pthread_mutex_unlock(&llMutex);

		add_ns_to_timespec(&nextRelease, TERMINAL_POLL_PERIOD_NS);
		if (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &nextRelease, NULL) != 0)
		{
			clock_gettime(CLOCK_MONOTONIC, &nextRelease);
		}
	}

	return NULL;
}

static void signal_handler(int signum)
{
	(void)signum;
	stopRequested = 1;
}

int main(void)
{
	printf("terminalTest: polling terminal every 15s and flushing LL on a loop\n");
	printf("Type characters and press Enter to add a sentence. Ctrl+C to exit.\n");

	signal(SIGINT, signal_handler);

	sentenceLL_init(&addHead, &encryptHead, &sendHead);
	terminal_init(&addHead);

	int stdinFlags = fcntl(STDIN_FILENO, F_GETFL, 0);
	if (stdinFlags >= 0)
	{
		if (fcntl(STDIN_FILENO, F_SETFL, stdinFlags | O_NONBLOCK) < 0)
		{
			perror("fcntl(F_SETFL, O_NONBLOCK)");
		}
	}
	else
	{
		perror("fcntl(F_GETFL)");
	}

	pthread_t readerThread;
	if (pthread_create(&readerThread, NULL, terminal_reader_thread, NULL) != 0)
	{
		perror("pthread_create");
		return EXIT_FAILURE;
	}

	struct timespec nextFlush;
	clock_gettime(CLOCK_MONOTONIC, &nextFlush);
	nextFlush.tv_sec += TERMINAL_FLUSH_PERIOD_SEC;

	while (!stopRequested)
	{
		struct timespec pollDelay = {0, TERMINAL_POLL_PERIOD_NS};
		(void)nanosleep(&pollDelay, NULL);

		struct timespec now;
		clock_gettime(CLOCK_MONOTONIC, &now);
		if ((now.tv_sec > nextFlush.tv_sec) ||
			((now.tv_sec == nextFlush.tv_sec) && (now.tv_nsec >= nextFlush.tv_nsec)))
		{
			pthread_mutex_lock(&llMutex);
			flush_linked_list();
			pthread_mutex_unlock(&llMutex);

			nextFlush = now;
			nextFlush.tv_sec += TERMINAL_FLUSH_PERIOD_SEC;
		}
	}

	pthread_mutex_lock(&llMutex);
	flush_linked_list();
	pthread_mutex_unlock(&llMutex);

	pthread_join(readerThread, NULL);
	printf("terminalTest exiting\n");
	return 0;
}
