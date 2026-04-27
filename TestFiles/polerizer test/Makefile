INCLUDE_DIRS = -I../../headers
LIB_DIRS =
CC=gcc

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= -lrt -lmbedcrypto -lwiringPi

HFILES= ../../$(wildcard headers/*.h)
CFILES= polerizerTest.c ../../$(wildcard sources/*.c)

SRCS= ${HFILES} ${CFILES}
OBJS= $(patsubst %.c,%.o,$(CFILES))

all:	polerizerTest

clean:
	-rm -f *.o ../../sources/*.o *.d
	-rm -f polerizerTest

distclean:
	-rm -f *.o ../../sources/*.o *.d

polerizerTest: ${OBJS}
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

depend:

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@
