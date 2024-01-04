OBJS += bitstream.o
OBJS += utils.o
OBJS += image.o
OBJS += debug.o

OBJS += galois.o
OBJS += reedsolomon.o
OBJS += runlength.o
OBJS += qrspec.o
OBJS += qrbch.o
OBJS += qrdata.o
OBJS += qrformat.o
OBJS += qrpayload.o
OBJS += qrversion.o

OBJS += qrean.o
OBJS += detector.o

OBJS += code_qr.o
OBJS += code_mqr.o
OBJS += code_rmqr.o
OBJS += code_tqr.o

OBJS += code_ean.o
OBJS += code_code39.o
OBJS += code_code93.o
OBJS += code_itf.o
OBJS += code_nw7.o

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

#CFLAGS += -D NO_MQR
#CFLAGS += -D NO_RMQR

#CFLAGS += -D NO_DEBUG

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
