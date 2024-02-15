.PHONY: all

BUILDDIR ?= $(abspath ../build/wasm)

SRCS += string.c
SRCS += stdlib.c
SRCS += math.c
SRCS += main.c
SRCS += ./tinymm/tinymm.c
SRCS += printf.c

CC = clang
AR = llvm-ar

CFLAGS += --target=wasm32
CFLAGS += -O3
CFLAGS += -nostdlib
CFLAGS += -DUSE_JS_MATH
#CFLAGS += -DNO_MALLOC
#CFLAGS += -DNO_DEBUG
CFLAGS += -DNO_PRINTF
CFLAGS += -I../src
CFLAGS += -I$(abspath .)
CFLAGS += -I$(abspath ./tinymm)

LDFLAGS += -Wl,--no-entry
LDFLAGS += -Wl,--export-all
LDFLAGS += -Wl,--allow-undefined
#LDFLAGS += -Wl,--initial-memory=6553600
#LDFLAGS += -Wl,--max-memory=6553600
#LDFLAGS += -Wl,-z,stack-size=3276800
#LDFLAGS += -Wl,--import-memory

all: $(BUILDDIR)/Qrean.js

$(BUILDDIR)/libqrean.a:
	@CC=$(CC) AR=$(AR) NO_SHLIB=1 CFLAGS="$(CFLAGS)" BUILDDIR=$(BUILDDIR) $(MAKE) -C ../src

$(BUILDDIR)/qrean.wasm.js: $(SRCS) $(BUILDDIR)/libqrean.a Makefile
	$(CC) $(CFLAGS) -nostartfiles $(SRCS) $(BUILDDIR)/libqrean.a -o $(BUILDDIR)/qrean.wasm $(LDFLAGS)
	deno run -A https://code4fukui.github.io/bin2js/bin2js.js $(BUILDDIR)/qrean.wasm

$(BUILDDIR)/Qrean.js: $(BUILDDIR)/qrean.wasm.js Qrean.ts Makefile
	deno bundle Qrean.ts $(BUILDDIR)/Qrean.js
	npx tsc --declaration --emitDeclarationOnly --declarationDir $(BUILDDIR) Qrean.ts
	mkdir -p ../dist/ && cp $(BUILDDIR)/Qrean.js $(BUILDDIR)/Qrean.d.ts ../dist/

clean:
	-rm -f $(BUILDDIR)/qrean.wasm $(BUILDDIR)/*.[do] $(BUILDDIR)/libqrean.a
