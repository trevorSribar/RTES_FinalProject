INCLUDE_DIRS = -I../../headers
LIB_DIRS =
CC=gcc

CDEFS=
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= -lrt -lmbedcrypto -lwiringPi

HFILES= ../../headers/servo.h ../../headers/generic.h
CFILES= servoTest.c ../../sources/servo.c

SRCS= ${HFILES} ${CFILES}
OBJS= $(patsubst %.c,%.o,$(CFILES))

all:	servoTest

clean:
	-rm -f *.o ../../sources/*.o *.d
	-rm -f servoTest

distclean:
	-rm -f *.o ../../sources/*.o *.d

servoTest: ${OBJS}
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

depend:

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@
