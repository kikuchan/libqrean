#ifndef __QR_BARCODE_H__
#define __QR_BARCODE_H__

#include <stdint.h>

#include "bitstream.h"
#include "utils.h"

#define MAX_BARCODE_BITS       (1024)
#define BARCODE_DEFAULT_HEIGHT (12)
#define BARCODE_DEFAULT_PADDING \
	{ 1, 10, 1, 10 }

typedef enum {
	BARCODE_TYPE_UPCA,
	BARCODE_TYPE_EAN13,
	BARCODE_TYPE_EAN8,

	BARCODE_TYPE_CODE39,
	BARCODE_TYPE_CODE93,
	BARCODE_TYPE_NW7,
	BARCODE_TYPE_ITF,
	// BARCODE_TYPE_CODE128,
} barcode_type_t;

typedef struct _barcode_t barcode_t;
struct _barcode_t {
	bitpos_t size;
	uint8_t bar_height;

	size_t width;
	size_t height;

	padding_t padding;

#if defined(USE_MALLOC_BUFFER) && defined(NO_MALLOC)
#error "Specify both of USE_MALLOC_BUFFER and NO_MALLOC doesn't make sense"
#endif
#ifdef USE_MALLOC_BUFFER
	uint8_t *buffer;
#else
	uint8_t buffer[BYTE_SIZE(MAX_BARCODE_BITS)];
#endif

#ifndef NO_CALLBACK
	bit_t (*write_pixel)(barcode_t *code, bitpos_t pos, bit_t v);
	bit_t (*read_pixel)(barcode_t *code, bitpos_t pos);
#endif
};

void barcode_init(barcode_t *code, bitpos_t bits);
void barcode_deinit(barcode_t *code);

barcode_t create_barcode();
barcode_t *new_barcode();

barcode_t create_barcode_with_string(barcode_type_t type, const char *src);
barcode_t *new_barcode_with_string(barcode_type_t type, const char *src);

bitpos_t barcode_write_string(barcode_t *code, barcode_type_t type, const char *src);

void barcode_dump(barcode_t *code);

// ----

bitstream_t barcode_create_bitstream(barcode_t *code, bitstream_iterator_t iter);

void barcode_set_size(barcode_t *code, bitpos_t bits);
void barcode_set_bar_height(barcode_t *code, bitpos_t h);
void barcode_set_padding(barcode_t *code, padding_t padding);
padding_t barcode_get_padding(barcode_t *code);

bitpos_t barcode_write_ean8_string(barcode_t *code, const char *src);
bitpos_t barcode_write_ean13_string(barcode_t *code, const char *src);
bitpos_t barcode_write_upca_string(barcode_t *code, const char *src);
bitpos_t barcode_write_code39_string(barcode_t *code, const char *src);
bitpos_t barcode_write_code93_string(barcode_t *code, const char *src);
bitpos_t barcode_write_nw7_string(barcode_t *code, const char *src);
bitpos_t barcode_write_itf_string(barcode_t *code, const char *src);
// bitpos_t barcode_write_code128_string(barcode_t *code, const char *src);

// --------------------------------------
void barcode_write_pixel(barcode_t *code, int_fast16_t x, int_fast16_t y, bit_t v);
bit_t barcode_read_pixel(barcode_t *code, int_fast16_t x, int_fast16_t y);

size_t barcode_write_bitmap(barcode_t *code, const void *buffer, size_t size, bitpos_t bpp);
size_t barcode_read_bitmap(barcode_t *code, void *buffer, size_t size, bitpos_t bpp);

#ifndef NO_CALLBACK
void barcode_on_write_pixel(barcode_t *code, bit_t (*write_pixel)(barcode_t *code, bitpos_t pos, bit_t v));
void barcode_on_read_pixel(barcode_t *code, bit_t (*read_pixel)(barcode_t *code, bitpos_t pos));
#endif

#endif /* __QR_BARCODE_H__ */
