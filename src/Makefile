BUILDDIR ?= $(abspath ../build/system)
DESTDIR ?= /usr

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

#CFLAGS += -g
#CFLAGS += -std=c2x
CFLAGS += -Wall # -Werror

CFLAGS += -Wno-misleading-indentation
CFLAGS += -fPIC

#CFLAGS += -D USE_MALLOC_BUFFER
#CFLAGS += -D NO_MALLOC
#CFLAGS += -D NO_PRINTF
#CFLAGS += -D NO_CALLBACK
#CFLAGS += -D NO_CANVAS_BUFFER

#CFLAGS += -D NO_KANJI_TABLE

#CFLAGS += -D NO_MQR
#CFLAGS += -D NO_RMQR

#CFLAGS += -D NO_DEBUG

LDFLAGS += -lm

OBJS = $(addprefix $(BUILDDIR)/,$(SRCS:.c=.o))
DEPS = $(OBJS:.o=.d)

VERSION := $(shell jq -r .version $(abspath ../package.json))
MAJORVERSION := $(shell echo "$(VERSION)" | sed 's,\..*,,')
LIBRARY = libqrean
TARGET = $(BUILDDIR)/$(LIBRARY)

CFLAGS += -DQREAN_VERSION=\"$(VERSION)\"

all: libs

libs:
	@$(MAKE) $(TARGET).a
ifndef NO_SHLIB
	@$(MAKE) $(TARGET).so.$(VERSION)
endif
ifdef DLL
	@$(MAKE) $(TARGET)-$(VERSION).dll
endif

$(TARGET).a: $(OBJS) Makefile
	$(AR) rcs $@ $^

$(TARGET).so.$(VERSION): $(OBJS) Makefile
ifndef NO_SHLIB
	$(CC) -shared -Wl,-soname,$(LIBRARY).so.$(MAJORVERSION) -o $@ $(OBJS)
	-ln -sf $(LIBRARY).so.$(VERSION) $(TARGET).so.$(MAJORVERSION)
	-ln -sf $(LIBRARY).so.$(VERSION) $(TARGET).so
endif

ifdef DLL
$(TARGET)-$(VERSION).dll: $(OBJS) Makefile
	$(CC) -shared -o $@ $(OBJS)
	-ln -sf $(LIBRARY)-$(VERSION).dll $(TARGET)-$(MAJORVERSION).dll
	-ln -sf $(LIBRARY)-$(VERSION).dll $(TARGET).dll
endif

-include $(DEPS)

$(BUILDDIR)/%.o: %.c Makefile
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c -MMD -MP $< -o $@

clean:
	rm -f $(BUILDDIR)/*.[do] $(TARGET)

install: libs
	install -s $(TARGET).so.$(VERSION) $(DESTDIR)/lib/
	install -m 644 $(TARGET).a $(DESTDIR)/lib/
	-ln -sf $(LIBRARY).so.$(VERSION) $(DESTDIR)/lib/$(LIBRARY).so.$(MAJORVERSION)
	-ln -sf $(LIBRARY).so.$(VERSION) $(DESTDIR)/lib/$(LIBRARY).so
	mkdir -p $(DESTDIR)/include/qrean/ && install -m 644 *.h $(DESTDIR)/include/qrean/

format:
	clang-format -i *.[ch]
