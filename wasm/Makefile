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
CFLAGS += -DNO_MALLOC
#CFLAGS += -DNO_DEBUG
CFLAGS += -DNO_PRINTF
CFLAGS += -I../src
CFLAGS += -I$(abspath .)
CFLAGS += -I$(abspath ./tinymm)

LDFLAGS += -Wl,--no-entry
LDFLAGS += -Wl,--export-all
LDFLAGS += -Wl,--allow-undefined
LDFLAGS += -Wl,--initial-memory=6553600
#LDFLAGS += -Wl,--max-memory=6553600
LDFLAGS += -Wl,-z,stack-size=655360

all: $(BUILDDIR)/qrean.wasm.js

$(BUILDDIR)/libqrean.a:
	@CC=$(CC) AR=$(AR) CFLAGS="$(CFLAGS)" BUILDDIR=$(BUILDDIR) $(MAKE) -C ../src

$(BUILDDIR)/qrean.wasm.js: $(BUILDDIR)/libqrean.a
	$(CC) $(CFLAGS) -nostartfiles $(SRCS) $(BUILDDIR)/libqrean.a -o $(BUILDDIR)/qrean.wasm $(LDFLAGS)
	deno run -A https://code4fukui.github.io/bin2js/bin2js.js $(BUILDDIR)/qrean.wasm
	deno bundle Qrean.ts $(BUILDDIR)/Qrean.js
	npx tsc --declaration --emitDeclarationOnly --declarationDir $(BUILDDIR) Qrean.ts
	cp $(BUILDDIR)/Qrean.js ../dist/

clean:
	-rm -f $(BUILDDIR)/qrean.wasm $(BUILDDIR)/*.[do] $(BUILDDIR)/libqrean.a
