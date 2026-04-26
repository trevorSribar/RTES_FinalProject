INCLUDE_DIRS = -Iheaders
LIB_DIRS =
CC=gcc

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= -lrt -lmbedcrypto -lwiringPi

HFILES= $(wildcard headers/*.h)
CFILES= seqgen.c $(wildcard sources/*.c)

SRCS= ${HFILES} ${CFILES}
OBJS= $(patsubst %.c,%.o,$(CFILES))

all:	seqgen

clean:
	-rm -f *.o sources/*.o *.d
	-rm -f seqgen

distclean:
	-rm -f *.o sources/*.o *.d

seqgen: ${OBJS}
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

depend:

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@
