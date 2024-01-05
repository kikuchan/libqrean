.PHONY: all

BUILDDIR ?= `pwd`/../build/wasm

SRCS += string.c
SRCS += stdlib.c
SRCS += math.c
SRCS += main.c
SRCS += ./tinymm/tinymm.c

CC = clang
AR = llvm-ar

CFLAGS += --target=wasm32
CFLAGS += -O3
CFLAGS += -nostdlib 
CFLAGS += -DUSE_JS_MATH 
CFLAGS += -DNO_MALLOC 
CFLAGS += -DNO_DEBUG 
CFLAGS += -DNO_PRINTF 
CFLAGS += -I../src
CFLAGS += -I`pwd`
CFLAGS += -I`pwd`/tinymm

LDFLAGS += -Wl,--no-entry
LDFLAGS += -Wl,--export-all
LDFLAGS += -Wl,--allow-undefined
LDFLAGS += -Wl,--initial-memory=655360
LDFLAGS += -Wl,--max-memory=655360

all: $(BUILDDIR)/qrean.wasm.js

$(BUILDDIR)/libqrean.a:
	CC=$(CC) AR=$(AR) CFLAGS="$(CFLAGS)" BUILDDIR=$(BUILDDIR) make -C ../src

$(BUILDDIR)/qrean.wasm.js: $(BUILDDIR)/libqrean.a
	$(CC) $(CFLAGS) -nostartfiles $(SRCS) $(BUILDDIR)/libqrean.a -o $(BUILDDIR)/qrean.wasm $(LDFLAGS)
	deno run -A https://code4fukui.github.io/bin2js/bin2js.js $(BUILDDIR)/qrean.wasm

clean:
	-rm -f $(BUILDDIR)/qrean.wasm $(BUILDDIR)/*.[do] $(BUILDDIR)/libqrean.a
