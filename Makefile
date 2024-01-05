BUILDDIR?=`pwd`/build/system

.PHONY: all clean examples cli wasm win32

all: cli

cli:
	BUILDDIR=$(BUILDDIR) make -C cli

install: cli
	BUILDDIR=$(BUILDDIR) make -C cli install

clean:
	BUILDDIR=$(BUILDDIR) make -C cli clean
	-rmdir $(BUILDDIR)

wasm:
	BUILDDIR=`pwd`/build/wasm/ make -C wasm clean all

win32:
	BUILDDIR=`pwd`/build/win32/ CC=i686-w64-mingw32-gcc AR=i686-w64-mingw32-ar make clean cli

dist: wasm win32
	mkdir -p dist
	cp build/wasm/qrean.wasm.js build/win32/*exe ./dist/

test: cli
	BUILDDIR=$(BUILDDIR) make -C tests
