BUILDDIR?=$(abspath ./build/system)

#CFLAGS += -DUSE_MALLOC_BUFFER
#CFLAGS += -DNO_MALLOC
#CFLAGS += -DNO_PRINTF
#CFLAGS += -DNO_CALLBACK
#CFLAGS += -DNO_CANVAS_BUFFER

#CFLAGS += -DNO_KANJI_TABLE

#CFLAGS += -DNO_MQR
#CFLAGS += -DNO_RMQR

#CFLAGS += -DNO_DEBUG

.PHONY: all clean examples cli wasm win32 mac

all: cli

cli:
	@BUILDDIR=$(BUILDDIR) CFLAGS="$(CFLAGS)" $(MAKE) -C cli

install: cli
	@BUILDDIR=$(BUILDDIR) CFLAGS="$(CFLAGS)" $(MAKE) -C src install
	@BUILDDIR=$(BUILDDIR) CFLAGS="$(CFLAGS)" $(MAKE) -C cli install

clean:
	@BUILDDIR=$(BUILDDIR) CFLAGS="$(CFLAGS)" $(MAKE) -C cli clean
	-rmdir $(BUILDDIR)

wasm:
	@BUILDDIR=$(abspath ./build/wasm) NO_SHLIB=1 CFLAGS="$(CFLAGS)" $(MAKE) -C wasm clean all

win32:
	@BUILDDIR=$(abspath ./build/win32) CC=i686-w64-mingw32-gcc AR=i686-w64-mingw32-ar NO_SHLIB=1 DLL=1 CFLAGS="$(CFLAGS)" $(MAKE) clean cli

test: cli
	@LD_LIBRARY_PATH=$(BUILDDIR) BUILDDIR=$(BUILDDIR) CFLAGS="$(CFLAGS)" $(MAKE) -C tests
