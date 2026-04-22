#define _GNU_SOURCE

#include "generic.h"
#include "terminal.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

#define TERMINAL_POLL_PERIOD_NS 30000000L
#define LL_PRINT_PERIOD_SEC 30

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

static void dump_ll_snapshot(void)
{
	uint16_t toEncrypt = sentenceLL_getNumSentencesToEncrypt();
	uint16_t toSend = sentenceLL_getNumSentencesToSend();
	uint16_t totalNodes = toEncrypt + toSend;

	printf("\n================ LL Snapshot ================\n");
	printf("addHead:    %p\n", (void *)addHead);
	printf("encryptHead:%p\n", (void *)encryptHead);
	printf("sendHead:   %p\n", (void *)sendHead);
	printf("toEncrypt=%u, toSend=%u, totalPopulatedNodes=%u\n",
		   toEncrypt,
		   toSend,
		   totalNodes);

	sentenceLinkedList_t *node = sendHead;
	for (uint16_t i = 0; i < totalNodes; i++)
	{
		if (node == NULL)
		{
			printf("node[%u]: NULL reached early\n", i);
			break;
		}

		printf("node[%u] @ %p\n", i, (void *)node);
		printf("  numCharacters: %u\n", node->numCharacters);
		printf("  sentence: ");
		fwrite(node->sentence, 1, node->numCharacters, stdout);
		printf("\n");

		printf("  next: %p\n", (void *)node->next);
		node = node->next;
	}
	printf("=============================================\n");
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
	printf("terminalTest: polling terminal every 30ms and dumping LL every 30s\n");
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

	while (!stopRequested)
	{
		sleep(LL_PRINT_PERIOD_SEC);

		pthread_mutex_lock(&llMutex);
		dump_ll_snapshot();
		pthread_mutex_unlock(&llMutex);
	}

	pthread_join(readerThread, NULL);
	printf("terminalTest exiting\n");
	return 0;
}
