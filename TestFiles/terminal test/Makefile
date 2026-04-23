INCLUDE_DIRS = -I../../headers -I../dummy\ files
CC = gcc

CFLAGS = -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS = -lrt -lpthread

HFILES = ../../headers/terminal.h ../../headers/generic.h ../../headers/sentenceLL.h ../../headers/encryption.h
CFILES = terminalInputOutputTest.c ../../sources/terminal.c ../../sources/sentenceLL.c
DUMMY_SRC = ../dummy\ files/encryption_dummy.c
DUMMY_OBJ = encryption_dummy.o

SRCS = ${HFILES} ${CFILES}
OBJS = $(patsubst %.c,%.o,$(CFILES)) $(DUMMY_OBJ)

all: terminalInputOutputTest

clean:
	-rm -f *.o ../../sources/*.o *.d
	-rm -f terminalInputOutputTest

distclean:
	-rm -f *.o ../../sources/*.o *.d

terminalInputOutputTest: ${OBJS}
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

$(DUMMY_OBJ): $(DUMMY_SRC)
	$(CC) $(CFLAGS) -c "$<" -o "$@"

.c.o:
	$(CC) $(CFLAGS) -c "$<" -o "$@"
