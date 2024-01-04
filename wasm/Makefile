.PHONY: all

all:
	clang --target=wasm32 \
	-O3 -nostdlib -nostartfiles \
	-DUSE_JS_MATH -DNO_MALLOC -DNO_DEBUG -DNO_PRINTF \
	-I../src -I./ \
	../src/bitstream.c \
	../src/debug.c \
	../src/qrdata.c \
	../src/qrean.c \
	../src/qrspec.c \
	../src/utils.c \
	../src/qrversion.c \
	../src/image.c \
	../src/qrpayload.c \
	../src/qrformat.c \
	../src/qrbch.c \
	../src/reedsolomon.c \
	../src/galois.c \
	../src/runlength.c \
	../src/code_mqr.c \
	../src/code_code39.c \
	../src/code_code93.c \
	../src/code_ean.c \
	../src/code_itf.c \
	../src/code_nw7.c \
	../src/code_qr.c \
	../src/code_rmqr.c \
	../src/code_tqr.c \
	./string.c \
	./stdlib.c \
	./math.c \
	./main.c \
	../src/detector.c \
	-Wl,--no-entry -Wl,--export-all -Wl,--allow-undefined \
	-Wl,--initial-memory=655360 -Wl,--max-memory=655360 \
	-o ./qrean.wasm
	deno run -A https://code4fukui.github.io/bin2js/bin2js.js qrean.wasm

clean:
	rm ./qrean.wasm
