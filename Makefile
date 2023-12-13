OBJS += hexdump.o
OBJS += bitstream.o
OBJS += qrmatrix.o
OBJS += formatinfo.o
OBJS += versioninfo.o
OBJS += qrstream.o
OBJS += galois.o
OBJS += qrdata.o
OBJS += reedsolomon.o

#CC=avr-gcc -mmcu=atmega32u4
#CC=clang
#CC=c++

SIZE = size

CFLAGS += -DDEBUG
#CFLAGS += -std=c2x
CFLAGS += -Wall # -Werror

#CFLAGS += -D USE_MALLOC_BUFFER
#CFLAGS += -D NO_MALLOC
#CFLAGS += -D NO_PRINTF
#CFLAGS += -D NO_CALLBACK
#CFLAGS += -D NO_QRMATRIX_BUFFER

all: size

main: $(OBJS) Makefile main.o
	$(CC) $(CFLAGS) -o main $(OBJS) main.o

libqr.a: ${OBJS}
	${AR} rcs libqr.a $(OBJS)

size: main libqr.a
	$(SIZE) main libqr.a

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o main libqr.a

run: main
	./main

format:
	clang-format -i *.[ch]
