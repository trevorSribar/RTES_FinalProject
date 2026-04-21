#include "terminal.h"

static int sentence_count = 0;
static sentenceLinkedList_t *temp = NULL;

void terminal_init(void)
{
	setvbuf(stdout, NULL, _IONBF, 0);
    temp = (sentenceLinkedList_t *) malloc(sizeof(sentenceLinkedList_t));
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
        sentenceLL_addSentence(temp, temp->sentence, sentence_count);
        temp = (sentenceLinkedList_t *) malloc(sizeof(sentenceLinkedList_t));
        sentence_count = 0;
        return 0;
    }

    temp->sentence[sentence_count] = (char) value;
    sentence_count++;
    if(sentence_count == SENTENCELL_SENTENCE_SIZE)
    {
        sentenceLL_addSentence(temp, temp->sentence, sentence_count);
        temp = (sentenceLinkedList_t *) malloc(sizeof(sentenceLinkedList_t));
        sentence_count = 0;
    }
	return 0;
}