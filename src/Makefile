OBJS += hexdump.o
OBJS += bitstream.o
OBJS += qrmatrix.o
OBJS += formatinfo.o
OBJS += versioninfo.o
OBJS += qrstream.o
OBJS += galois.o
OBJS += qrdata.o
OBJS += reedsolomon.o

OBJS += barcode.o
OBJS += barcode_code39.o
OBJS += barcode_code93.o
OBJS += barcode_ean.o
OBJS += barcode_itf.o
OBJS += barcode_nw7.o

#CC=avr-gcc -mmcu=atmega32u4
#CC=clang
#CC=c++

SIZE = size

CFLAGS += -g
CFLAGS += -DDEBUG
#CFLAGS += -std=c2x
CFLAGS += -Wall # -Werror

#CFLAGS += -D USE_MALLOC_BUFFER
#CFLAGS += -D NO_MALLOC
#CFLAGS += -D NO_PRINTF
#CFLAGS += -D NO_CALLBACK
#CFLAGS += -D NO_QRMATRIX_BUFFER
#CFLAGS += -D NO_BARCODE_BUFFER

all: size decode

main: $(OBJS) Makefile main.o
	$(CC) $(CFLAGS) -o main $(OBJS) main.o

decode: $(OBJS) Makefile decode.o pngle/*.o
	$(CC) $(CFLAGS) -o decode $(OBJS) decode.o pngle/pngle.c pngle/miniz.c -lm

libqrean.a: ${OBJS}
	${AR} rcs libqrean.a $(OBJS)

size: main libqrean.a
	$(SIZE) main libqrean.a

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o main libqrean.a

run: main
	./main

format:
	clang-format -i *.[ch]
