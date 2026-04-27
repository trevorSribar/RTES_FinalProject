#include "terminal.h"
#include <sys/select.h>
#include <unistd.h>

static int sentence_length = 0;
static char input_buffer[SENTENCELL_SENTENCE_SIZE];
static sentenceLinkedList_t **head = NULL;

void terminal_init(sentenceLinkedList_t **linkedhead)
{
	setvbuf(stdout, NULL, _IONBF, 0);
    head = linkedhead;
}

int terminal_read_char()
{
    fd_set readfds;
    struct timeval timeout = {.tv_sec = 0, .tv_usec = 0};

    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    int ready = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);
    if (ready < 0)
    {
        return -1;
    }

    if (ready == 0 || !FD_ISSET(STDIN_FILENO, &readfds))
    {
        // this is a weird early return that is success, so I don't really know what this is for
        return 0;
    }

    int value = getchar();
    printf("%c, %d\t",value, value); // remove this
	if (value == EOF)
	{
		return -1;
	}

    if(value == '\n' || value == '\r')
    {
        if (sentence_length == 0)
        {
            return 0;
        }
        sentenceLL_addSentence(head, input_buffer, sentence_length);
        sentence_length = 0;
        return 0;
    }

    input_buffer[sentence_length] = (char) value;
    sentence_length++;
    if(sentence_length == SENTENCELL_SENTENCE_SIZE)
    {
        sentenceLL_addSentence(head, input_buffer, sentence_length);
        sentence_length = 0;
        printf("\n\r >> ");
    }
	return 0;
}

void terminal_print_and_delete_DecryptedSentence(sentenceLinkedList_t **tail){
    char old_sentence[SENTENCELL_SENTENCE_SIZE];
    uint8_t old_sentence_length;
    sentenceLL_getSentence(tail, old_sentence, &old_sentence_length);

    printf("Printing decrypted sentence: \n");
    for(uint8_t i = 0; i < old_sentence_length; i++){
        printf("%c", old_sentence[i]);
    }
    printf("\n");

    sentenceLL_removeSentence(tail);
}