#include <stdlib.h>
#include <string.h>

#include "barcode.h"
#include "bitstream.h"

void barcode_init(barcode_t *code, barcode_type_t type) {
	switch (type) {
	case BARCODE_TYPE_UPCA:
	case BARCODE_TYPE_EAN13:
	case BARCODE_TYPE_EAN8:
		barcode_ean13_init(code, type);
		break;

	case BARCODE_TYPE_CODE39:
		barcode_code39_init(code);
		break;

	case BARCODE_TYPE_CODE93:
		barcode_code93_init(code);
		break;

	case BARCODE_TYPE_NW7:
		barcode_nw7_init(code);
		break;

	case BARCODE_TYPE_ITF:
		barcode_itf_init(code);
		break;
	}
}

void barcode_deinit(barcode_t *code) {
	switch (code->type) {
	case BARCODE_TYPE_UPCA:
	case BARCODE_TYPE_EAN13:
	case BARCODE_TYPE_EAN8:
		return barcode_ean13_deinit(code);

	case BARCODE_TYPE_CODE39:
		return barcode_code39_deinit(code);

	case BARCODE_TYPE_CODE93:
		return barcode_code93_deinit(code);

	case BARCODE_TYPE_NW7:
		return barcode_nw7_deinit(code);

	case BARCODE_TYPE_ITF:
		return barcode_itf_deinit(code);
	}
}

bitpos_t barcode_put_string(barcode_t *code, const char *src) {
	switch (code->type) {
	case BARCODE_TYPE_UPCA:
	case BARCODE_TYPE_EAN13:
	case BARCODE_TYPE_EAN8:
		return barcode_ean13_put_string(code, src);

	case BARCODE_TYPE_CODE39:
		return barcode_code39_put_string(code, src);

	case BARCODE_TYPE_CODE93:
		return barcode_code93_put_string(code, src);

	case BARCODE_TYPE_NW7:
		return barcode_nw7_put_string(code, src);

	case BARCODE_TYPE_ITF:
		return barcode_itf_put_string(code, src);
	}
	return 0;
}

void barcode_buffer_init(barcode_t *code, bitpos_t bits) {
	code->size = bits && bits < MAX_BARCODE_BITS ? bits : MAX_BARCODE_BITS;
#ifndef NO_BARCODE_BUFFER
#if defined(USE_MALLOC_BUFFER) && !defined(NO_MALLOC)
	code->buffer = (uint8_t *)malloc(BYTE_SIZE(code->size));
#endif
	memset(code->buffer, 0, BYTE_SIZE(code->size));
#endif
}

void barcode_buffer_deinit(barcode_t *code) {
#ifndef NO_BARCODE_BUFFER
#ifdef USE_MALLOC_BUFFER
	free(code->buffer);
#endif
#endif
}

barcode_t create_barcode(barcode_type_t type) {
	barcode_t code = {};
	barcode_init(&code, type);
	return code;
}

void barcode_destroy(barcode_t *code) {
	barcode_deinit(code);
}

barcode_t *new_barcode(barcode_type_t type) {
#ifndef NO_MALLOC
	barcode_t *code = (barcode_t *)malloc(sizeof(barcode_t));
	if (!code) return NULL;
	barcode_init(code, type);
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
	barcode_init(&code, type);
	barcode_put_string(&code, src);
	return code;
}

barcode_t *new_barcode_with_string(barcode_type_t type, const char *src) {
#ifndef NO_MALLOC
	barcode_t *code = new_barcode(type);
	if (code) barcode_put_string(code, src);
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
	// bitstream_on_put_bit(&bs, barcode_put_bit_at, qr);
	// bitstream_on_get_bit(&bs, barcode_get_bit_at, qr);
	return bs;
}

void barcode_dump(barcode_t *code, int height) {
#ifndef NO_PRINTF
	const char *white = "\033[47m \033[m";
	const char *black = " ";
	bitstream_t bs = barcode_create_bitstream(code, bitstream_loop_iter);

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < code->size; x++) {
			printf("%s", bitstream_get_bit(&bs) ? black : white);
		}
		printf("\n");
	}
#endif
}
