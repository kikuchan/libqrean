#include <stdint.h>

#include "bitstream.h"

#define MAX_BARCODE_BITS (1024)
#define BYTE_SIZE(bits)  ((bits + 7) / 8)

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

typedef struct {
	barcode_type_t type;
	bitpos_t size;
#if defined(USE_MALLOC_BUFFER) && defined(NO_MALLOC)
#error "Specify both of USE_MALLOC_BUFFER and NO_MALLOC doesn't make sense"
#endif
#ifdef USE_MALLOC_BUFFER
	uint8_t *buffer;
#else
	uint8_t buffer[BYTE_SIZE(MAX_BARCODE_BITS)];
#endif
} barcode_t;

void barcode_init(barcode_t *code, barcode_type_t type);
void barcode_deinit(barcode_t *code);

barcode_t create_barcode(barcode_type_t type);
barcode_t *new_barcode(barcode_type_t type);

barcode_t create_barcode_with_string(barcode_type_t type, const char *src);
barcode_t *new_barcode_with_string(barcode_type_t type, const char *src);

bitpos_t barcode_write_string(barcode_t *code, const char *src);

void barcode_dump(barcode_t *code, uint8_t height);

// ----

void barcode_buffer_init(barcode_t *code, bitpos_t bits);
void barcode_buffer_deinit(barcode_t *code);

bitstream_t barcode_create_bitstream(barcode_t *code, bitstream_iterator_t iter);
void barcode_set_size(barcode_t *code, bitpos_t bits);

void barcode_ean13_init(barcode_t *code, barcode_type_t type);
void barcode_code39_init(barcode_t *code);
void barcode_code93_init(barcode_t *code);
void barcode_nw7_init(barcode_t *code);
void barcode_itf_init(barcode_t *code);
// void barcode_code128_init(barcode_t *code);

void barcode_ean13_deinit(barcode_t *code);
void barcode_code39_deinit(barcode_t *code);
void barcode_code93_deinit(barcode_t *code);
void barcode_nw7_deinit(barcode_t *code);
void barcode_itf_deinit(barcode_t *code);
// void barcode_code128_deinit(barcode_t *code);

bitpos_t barcode_ean13_write_string(barcode_t *code, const char *src);
bitpos_t barcode_code39_write_string(barcode_t *code, const char *src);
bitpos_t barcode_code93_write_string(barcode_t *code, const char *src);
bitpos_t barcode_nw7_write_string(barcode_t *code, const char *src);
bitpos_t barcode_itf_write_string(barcode_t *code, const char *src);
// bitpos_t barcode_code128_write_string(barcode_t *code, const char *src);
