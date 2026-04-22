INCLUDE_DIRS = -I../../headers -I../dummy\ files
CC = gcc

CDEFS =
CFLAGS = -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS = -lrt -lpthread

HFILES = ../../headers/terminal.h ../../headers/generic.h ../../headers/sentenceLL.h ../../headers/encryption.h
CFILES = terminalTest.c ../../sources/terminal.c ../../sources/sentenceLL.c

SRCS = ${HFILES} ${CFILES}
OBJS = $(patsubst %.c,%.o,$(CFILES))

all: terminalTest

clean:
	-rm -f *.o ../../sources/*.o *.d
	-rm -f terminalTest

distclean:
	-rm -f *.o ../../sources/*.o *.d

terminalTest: ${OBJS}
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

depend:

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@
