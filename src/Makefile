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

SRCS += image.c
SRCS += detector.c

#CC=avr-gcc -mmcu=atmega32u4
#CC=clang
#CC=c++

SIZE = size

#CFLAGS += -std=c2x
CFLAGS += -Wall # -Werror

CFLAGS += -O3

CFLAGS += -Wno-misleading-indentation
CFLAGS += -fPIC

LDFLAGS += -lm
ifndef NO_DEBUG
CFLAGS += -g
endif

OBJS = $(addprefix $(BUILDDIR)/,$(SRCS:.c=.o))
DEPS = $(OBJS:.o=.d)

VERSION := $(shell grep version $(abspath ../package.json) | head -1 | awk '{print $$2}' | tr -d ',"')
MAJORVERSION := $(word 1,$(subst ., ,$(VERSION)))
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
ifndef NO_PKGCONFIG
	mkdir -p $(DESTDIR)/lib/pkgconfig
	sed 's,@prefix@,$(DESTDIR),' libqrean.pc.in > $(DESTDIR)/lib/pkgconfig/libqrean.pc
endif
	install -s $(TARGET).so.$(VERSION) $(DESTDIR)/lib/
	install -m 644 $(TARGET).a $(DESTDIR)/lib/
	-ln -sf $(LIBRARY).so.$(VERSION) $(DESTDIR)/lib/$(LIBRARY).so.$(MAJORVERSION)
	-ln -sf $(LIBRARY).so.$(VERSION) $(DESTDIR)/lib/$(LIBRARY).so
	echo '#include "qrean/qrean.h"' > $(DESTDIR)/include/qrean.h
	mkdir -p $(DESTDIR)/include/qrean/ && install -m 644 *.h $(DESTDIR)/include/qrean/

format:
	clang-format -i *.[ch]
