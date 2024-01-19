BUILDDIR?=$(abspath ./build/system)

.PHONY: all clean examples cli wasm win32 mac

all: cli

cli:
	@BUILDDIR=$(BUILDDIR) $(MAKE) -C cli

install: cli
	@BUILDDIR=$(BUILDDIR) $(MAKE) -C src install
	@BUILDDIR=$(BUILDDIR) $(MAKE) -C cli install

clean:
	@BUILDDIR=$(BUILDDIR) $(MAKE) -C cli clean
	-rmdir $(BUILDDIR)

wasm:
	@BUILDDIR=$(abspath ./build/wasm) NO_SHLIB=1 $(MAKE) -C wasm clean all

win32:
	@BUILDDIR=$(abspath ./build/win32) CC=i686-w64-mingw32-gcc AR=i686-w64-mingw32-ar NO_SHLIB=1 DLL=1 $(MAKE) clean cli

mac: cli
	cp build/system/qrean build/system/qrean-detect ./dist/

dist: wasm win32
	mkdir -p dist
	cp build/wasm/Qrean.* build/win32/*exe ./dist/

test: cli
	@LD_LIBRARY_PATH=$(BUILDDIR) BUILDDIR=$(BUILDDIR) $(MAKE) -C tests
