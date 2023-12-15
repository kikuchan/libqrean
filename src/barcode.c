#include <stdlib.h>
#include <string.h>

#include "barcode.h"
#include "bitstream.h"
#include "utils.h"

void barcode_init(barcode_t *code, bitpos_t bits) {
	barcode_set_size(code, bits);
	barcode_set_bar_height(code, BARCODE_DEFAULT_HEIGHT);

	padding_t padding = {BARCODE_DEFAULT_PADDING};
	barcode_set_padding(code, padding);

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

bitpos_t barcode_write_string(barcode_t *code, barcode_type_t type, const char *src) {
	switch (type) {
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
	barcode_write_string(&code, type, src);
	return code;
}

barcode_t *new_barcode_with_string(barcode_type_t type, const char *src) {
#ifndef NO_MALLOC
	barcode_t *code = new_barcode();
	if (code) barcode_write_string(code, type, src);
	return code;
#else
	return NULL;
#endif
}

void barcode_set_size(barcode_t *code, bitpos_t size) {
	code->size = size && size < MAX_BARCODE_BITS ? size : MAX_BARCODE_BITS;
}

void barcode_set_bar_height(barcode_t *code, bitpos_t h) {
	code->bar_height = h;
}

void barcode_set_padding(barcode_t *code, padding_t padding) {
	code->padding = padding;
	code->width = code->padding.l + code->size + code->padding.r;
	code->height = code->padding.t + code->bar_height + code->padding.b;
}
padding_t barcode_get_padding(barcode_t *code) {
	return code->padding;
}

static void barcode_write_bit_at(bitstream_t *bs, bitpos_t pos, void *opaque, bit_t v) {
	barcode_t *code = (barcode_t *)opaque;
#ifndef NO_CALLBACK
	if (code->write_pixel) {
		if (code->write_pixel(code, pos, v)) return;
	}
#endif

#ifndef NO_QRMATRIX_BUFFER
	WRITE_BIT(code->buffer, pos, v);
#endif
}

static bit_t barcode_read_bit_at(bitstream_t *bs, bitpos_t pos, void *opaque) {
	barcode_t *code = (barcode_t *)opaque;
#ifndef NO_CALLBACK
	if (code->read_pixel) {
		return code->read_pixel(code, pos);
	} else
#endif
	{
#ifndef NO_QRMATRIX_BUFFER
		return READ_BIT(code->buffer, pos);
#else
		return 0;
#endif
	}
}

void barcode_write_pixel(barcode_t *code, int x, int y, bit_t v) {
	if (x < 0) return;
	if (y < 0) return;
	if (x >= code->size) return;
	if (y >= code->bar_height) return;

	barcode_write_bit_at(NULL, x, code, v);
}

bit_t barcode_read_pixel(barcode_t *code, int x, int y) {
	if (x < 0) return 0;
	if (y < 0) return 0;
	if (x >= code->size) return 0;
	if (y >= code->bar_height) return 0;

	return barcode_read_bit_at(NULL, x, code);
}

bitstream_t barcode_create_bitstream(barcode_t *code, bitstream_iterator_t iter) {
	bitstream_t bs = create_bitstream(code->buffer, code->size, iter, code);
	bitstream_on_write_bit(&bs, barcode_write_bit_at, code);
	bitstream_on_read_bit(&bs, barcode_read_bit_at, code);
	return bs;
}

void barcode_dump(barcode_t *code) {
#ifndef NO_PRINTF
	const char *dots[4] = { "\u2588", "\u2580", "\u2584", " " };

	int_fast16_t sx = -code->padding.l;
	int_fast16_t sy = -code->padding.t;
	int_fast16_t ex = (int_fast16_t)code->size + code->padding.r;
	int_fast16_t ey = (int_fast16_t)code->bar_height + code->padding.b;
	for (int_fast16_t y = sy; y < ey; y += 2) {
		for (int_fast16_t x = sx; x < ex; x++) {
			bit_t u = barcode_read_pixel(code, x, y + 0);
			bit_t l = y + 1 >= ey ? 1 : barcode_read_pixel(code, x, y + 1);

			printf("%s", dots[((u << 1) | l) & 3]);
		}
		printf("\n");
	}
#endif
}

static bitpos_t bitmap_iter(bitstream_t *bs, bitpos_t i, void *opaque) {
	barcode_t *code = (barcode_t *)opaque;

	if (i >= code->width * code->height) return BITPOS_END;

	ssize_t x = (ssize_t)(i % code->width) - code->padding.l;

	return ((x) < 0 || (x) > (code)->size) ? BITPOS_BLANK : (x)&BITPOS_MASK;
}

size_t barcode_read_bitmap(barcode_t *code, void *buffer, size_t size, bitpos_t bpp) {
	bitstream_t src = barcode_create_bitstream(code, bitmap_iter);
	bitstream_t dst = create_bitstream(buffer, size * 8, NULL, NULL);

	while (!bitstream_is_end(&src) && !bitstream_is_end(&dst)) {
		uint32_t v = bitstream_read_bit(&src) ? 0x00000000 : 0xffffffff;
		bitstream_write_bits(&dst, v, bpp);
	}
	return bitstream_tell(&dst) / 8;
}

size_t barcode_write_bitmap(barcode_t *code, const void *buffer, size_t size, bitpos_t bpp) {
	bitstream_t dst = barcode_create_bitstream(code, bitmap_iter);
	bitstream_t src = create_bitstream(buffer, size * 8, NULL, NULL);

	while (!bitstream_is_end(&src) && !bitstream_is_end(&dst)) {
		bit_t v = bitstream_read_bits(&src, bpp) ? 0 : 1;
		bitstream_write_bit(&dst, v);
	}
	return bitstream_tell(&src) / 8;
}

#ifndef NO_CALLBACK
void barcode_on_write_pixel(barcode_t *code, bit_t (*write_pixel)(barcode_t *code, bitpos_t pos, bit_t v)) {
	code->write_pixel = write_pixel;
}
void barcode_on_read_pixel(barcode_t *code, bit_t (*read_pixel)(barcode_t *code, bitpos_t pos)) {
	code->read_pixel = read_pixel;
}
#endif
