PREFIX?=/usr/bin
BUILDDIR?=$(abspath ../../build/system)
QREAN?=$(abspath ../../src)

CFLAGS+=-I$(QREAN)
LDFLAGS += -L$(BUILDDIR) -Wl,-rpath,$(BUILDDIR) -lqrean -lm

NAME=libqrean

ifdef DLL
LIBSUFFIX := .dll
else
LIBSUFFIX := .so
endif

all: $(BUILDDIR)/$(MAIN)

$(BUILDDIR)/$(MAIN): Makefile $(SRCS) $(BUILDDIR)/$(NAME)$(LIBSUFFIX)
	$(CC) -o $(BUILDDIR)/$(MAIN) $(CFLAGS) $(SRCS) $(LDFLAGS)

$(BUILDDIR)/$(NAME)$(LIBSUFFIX): $(QREAN)
	@$(MAKE) -C $(QREAN)

format:
	clang-format -i $(MAIN).c

clean:
	rm -f $(BUILDDIR)/$(MAIN) $(BUILDDIR)/$(MAIN).exe $(BUILDDIR)/*.[do] $(BUILDDIR)/libqrean.*

install: $(BUILDDIR)/$(MAIN)
	install -m 0755 $(BUILDDIR)/$(MAIN) $(PREFIX)
	patchelf --remove-rpath $(PREFIX)/$(MAIN)
