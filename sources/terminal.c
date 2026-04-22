#include "terminal.h"

static int sentence_length = 0;
static char input_buffer[SENTENCELL_SENTENCE_SIZE];
static sentenceLinkedList_t *head = NULL;

void terminal_init(sentenceLinkedList_t *linkedhead)
{
	setvbuf(stdout, NULL, _IONBF, 0);
    head = linkedhead;
}

int terminal_read_char()
{
    int value;

	if (out_char == NULL)
	{
		return -1;
	}

	value = getchar();
	if (value == EOF)
	{
		return -1;
	}

    if(value == '\n')
    {
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
    }
	return 0;
}