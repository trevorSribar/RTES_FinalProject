#include "terminal.h"

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
    int value = getchar();
	if (value == EOF)
	{
		return -1;
	}

    if(value == '\n' || value == '\r')
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

void terminal_print_and_delete_DecryptedSentence(sentenceLinkedList_t **tail){
    char *old_sentence;
    uint8_t old_sentence_length;
    char nonce[ENCRYPTION_NONCE_LENGTH];
    sentenceLL_getSentence(tail, old_sentence, &old_sentence_length);

    printf("Printing decrypted sentence: \n");
    for(uint8_t i = 0; i < old_sentence_length; i++){
        printf("%c", old_sentence[i]);
    }
    printf("\n");

    sentenceLL_removeSentence(tail);
}