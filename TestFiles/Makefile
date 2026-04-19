INCLUDE_DIRS = -I../headers
LIB_DIRS =
CC=gcc

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= -lrt -lmbedcrypto

HFILES= ../headers/encryption.h ../headers/generic.h ../headers/sentenceLL.h
CFILES= encryptDecryptTest.c ../sources/encryption.c ../sources/sentenceLL.c

SRCS= ${HFILES} ${CFILES}
OBJS= $(patsubst %.c,%.o,$(CFILES))

all:	encryptDecryptTest

clean:
	-rm -f *.o ../sources/*.o *.d
	-rm -f encryptDecryptTest

distclean:
	-rm -f *.o ../sources/*.o *.d

encryptDecryptTest: ${OBJS}
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

depend:

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@
