PREFIX ?= /usr/bin
BUILDDIR?=$(abspath ../../build/system)
QREAN?=$(abspath ../../src)

MAIN=qrean-detect

SRCS += $(MAIN).c
SRCS += pngle.c
SRCS += miniz.c

CFLAGS+=-I$(QREAN)
LDFLAGS += $(BUILDDIR)/libqrean.a -lm

all: $(BUILDDIR)/$(MAIN)

$(BUILDDIR)/$(MAIN): $(SRCS) $(BUILDDIR)/libqrean.a
	$(CC) -o $(BUILDDIR)/$(MAIN) $(CFLAGS) $(SRCS) $(LDFLAGS)

$(BUILDDIR)/libqrean.a: $(QREAN)
	@$(MAKE) -C $(QREAN)

format:
	clang-format -i $(MAIN).c

clean:
	rm -f $(BUILDDIR)/$(MAIN) $(BUILDDIR)/$(MAIN).exe $(BUILDDIR)/*.[do] $(BUILDDIR)/libqrean.a

install: $(BUILDDIR)/$(MAIN)
	install -m 0755 $(BUILDDIR)/$(MAIN) $(PREFIX)
