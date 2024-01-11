BUILDDIR ?= $(abspath ../build/system)

SRCS += bitstream.c
SRCS += utils.c
SRCS += debug.c

SRCS += galois.c
SRCS += reedsolomon.c
SRCS += runlength.c
SRCS += qrspec.c
SRCS += qrbch.c
SRCS += qrdata.c
SRCS += qrformat.c
SRCS += qrpayload.c
SRCS += qrversion.c
SRCS += qrkanji.c

SRCS += qrean.c

SRCS += code_qr.c
SRCS += code_mqr.c
SRCS += code_rmqr.c
SRCS += code_tqr.c

SRCS += code_ean.c
SRCS += code_code39.c
SRCS += code_code93.c
SRCS += code_itf.c
SRCS += code_nw7.c

## Works only without NO_MALLOC
SRCS += image.c
SRCS += detector.c

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
#CFLAGS += -D NO_CANVAS_BUFFER

#CFLAGS += -D NO_MQR
#CFLAGS += -D NO_RMQR

#CFLAGS += -D NO_DEBUG

LDFLAGS += -lm

OBJS = $(addprefix $(BUILDDIR)/,$(SRCS:.c=.o))
DEPS = $(OBJS:.o=.d)

TARGET = $(BUILDDIR)/libqrean.a

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) rcs $@ $^

-include $(DEPS)

$(BUILDDIR)/%.o: %.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c -MMD -MP $< -o $@

clean:
	rm -f $(BUILDDIR)/*.[do] $(TARGET)

format:
	clang-format -i *.[ch]
