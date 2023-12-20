OBJS += bitstream.o
OBJS += hexdump.o
OBJS += utils.o
OBJS += image.o
OBJS += debug.o

OBJS += galois.o
OBJS += qrdata.o
OBJS += qrformat.o
OBJS += qrmatrix.o
OBJS += qrpayload.o
OBJS += qrversion.o
OBJS += reedsolomon.o
OBJS += runlength.o
OBJS += qrdetector.o
OBJS += qrspec.o

OBJS += barcode.o
OBJS += barcode_code39.o
OBJS += barcode_code93.o
OBJS += barcode_ean.o
OBJS += barcode_itf.o
OBJS += barcode_nw7.o


#CFLAGS += -D USE_IMAGE_PNG
#OBJS += miniz.o

#CC=avr-gcc -mmcu=atmega32u4
#CC=clang
#CC=c++

SIZE = size

CFLAGS += -g
#CFLAGS += -std=c2x
CFLAGS += -Wall # -Werror

CFLAGS += -Wno-misleading-indentation

#CFLAGS += -D USE_MALLOC_BUFFER
#CFLAGS += -D NO_MALLOC
#CFLAGS += -D NO_PRINTF
#CFLAGS += -D NO_CALLBACK
#CFLAGS += -D NO_QRMATRIX_BUFFER
#CFLAGS += -D NO_BARCODE_BUFFER

#CFLAGS += -D NO_DEBUG
CFLAGS += -D DEBUG_QRDATA

all: size

libqrean.a: ${OBJS}
	${AR} rcs libqrean.a $(OBJS)

size: libqrean.a
	$(SIZE) libqrean.a

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o libqrean.a

format:
	clang-format -i *.[ch]
