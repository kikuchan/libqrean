#include <stdlib.h>
#include <string.h>

#include "barcode.h"
#include "bitstream.h"

void barcode_init(barcode_t *code, bitpos_t bits) {
	code->size = bits && bits < MAX_BARCODE_BITS ? bits : MAX_BARCODE_BITS;
#ifndef NO_BARCODE_BUFFER
#if defined(USE_MALLOC_BUFFER) && !defined(NO_MALLOC)
	code->buffer = (uint8_t *)malloc(BYTE_SIZE(code->size));
#endif
	memset(code->buffer, 0, BYTE_SIZE(code->size));
#endif
}

void barcode_deinit(barcode_t *code) {
#ifndef NO_BARCODE_BUFFER
#ifdef USE_MALLOC_BUFFER
	free(code->buffer);
#endif
#endif
}


bitpos_t barcode_write_string(barcode_t *code, const char *src) {
	switch (code->type) {
	case BARCODE_TYPE_UPCA:
	case BARCODE_TYPE_EAN13:
	case BARCODE_TYPE_EAN8:
		return barcode_write_ean13_string(code, src);

	case BARCODE_TYPE_CODE39:
		return barcode_write_code39_string(code, src);

	case BARCODE_TYPE_CODE93:
		return barcode_write_code93_string(code, src);

	case BARCODE_TYPE_NW7:
		return barcode_write_nw7_string(code, src);

	case BARCODE_TYPE_ITF:
		return barcode_write_itf_string(code, src);
	}
	return 0;
}

barcode_t create_barcode() {
	barcode_t code = {};
	barcode_init(&code, 0);
	return code;
}

void barcode_destroy(barcode_t *code) {
	barcode_deinit(code);
}

barcode_t *new_barcode() {
#ifndef NO_MALLOC
	barcode_t *code = (barcode_t *)malloc(sizeof(barcode_t));
	if (!code) return NULL;
	barcode_init(code, 0);
	return code;
#else
	return NULL;
#endif
}

void barcode_free(barcode_t *code) {
#ifndef NO_MALLOC
	barcode_deinit(code);
	free(code);
#endif
}

barcode_t create_barcode_with_string(barcode_type_t type, const char *src) {
	barcode_t code = {};
	barcode_init(&code, 0);
	barcode_write_string(&code, src);
	return code;
}

barcode_t *new_barcode_with_string(barcode_type_t type, const char *src) {
#ifndef NO_MALLOC
	barcode_t *code = new_barcode();
	if (code) barcode_write_string(code, src);
	return code;
#else
	return NULL;
#endif
}

void barcode_set_size(barcode_t *code, bitpos_t size) {
	code->size = size < code->size ? size : code->size;
}

bitstream_t barcode_create_bitstream(barcode_t *code, bitstream_iterator_t iter) {
	bitstream_t bs = create_bitstream(code->buffer, code->size, iter, code);
	// bitstream_on_write_bit(&bs, barcode_write_bit_at, qr);
	// bitstream_on_read_bit(&bs, barcode_read_bit_at, qr);
	return bs;
}

void barcode_dump(barcode_t *code, uint8_t height) {
#ifndef NO_PRINTF
	const char *white = "\033[47m \033[m";
	const char *black = " ";
	bitstream_t bs = barcode_create_bitstream(code, bitstream_loop_iter);

	for (uint8_t y = 0; y < height; y++) {
		for (uint8_t x = 0; x < code->size; x++) {
			printf("%s", bitstream_read_bit(&bs) ? black : white);
		}
		printf("\n");
	}
#endif
}
