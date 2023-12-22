#ifndef __QR_QRMATRIX_H__
#define __QR_QRMATRIX_H__

#include <stdio.h>
#include <stdint.h>

#include "bitstream.h"
#include "qrformat.h"
#include "qrpayload.h"
#include "qrtypes.h"
#include "qrversion.h"
#include "utils.h"

#define MAX_QR_VERSION (QR_VERSION_40)

#define SYMBOL_SIZE_FOR(version) (17 + (version)*4)

#define QR_BUFFER_SIDE_LENGTH SYMBOL_SIZE_FOR(MAX_QR_VERSION)
#define QR_BUFFER_SIZE        ((QR_BUFFER_SIDE_LENGTH * QR_BUFFER_SIDE_LENGTH + 7) / 8)

typedef struct _qrmatrix_t qrmatrix_t;

struct _qrmatrix_t {
	qr_version_t version;
	qr_errorlevel_t level;
	qr_maskpattern_t mask;
	uint8_t symbol_size;

	uint8_t width;
	uint8_t height;
	padding_t padding;

#if defined(USE_MALLOC_BUFFER) && defined(NO_MALLOC)
#error "Specify both of USE_MALLOC_BUFFER and NO_MALLOC doesn't make sense"
#endif
#ifdef USE_MALLOC_BUFFER
	uint8_t *buffer;
#else
	uint8_t buffer[QR_BUFFER_SIZE];
#endif

#ifndef NO_CALLBACK
	bit_t (*write_pixel)(qrmatrix_t *qr, bitpos_t x, bitpos_t y, bitpos_t pos, bit_t v, void *opaque);
	void *opaque_write;
	bit_t (*read_pixel)(qrmatrix_t *qr, bitpos_t x, bitpos_t y, bitpos_t pos, void *opaque);
	void *opaque_read;
#endif
};

bit_t qrmatrix_init(qrmatrix_t *qr);
void qrmatrix_deinit(qrmatrix_t *qr);

qrmatrix_t create_qrmatrix();
qrmatrix_t create_qrmatrix_for_string(qr_version_t version, qr_errorlevel_t level, qr_maskpattern_t mask, const char *src);
void qrmatrix_destroy(qrmatrix_t *qr);

qrmatrix_t *new_qrmatrix();
qrmatrix_t *new_qrmatrix_for_string(qr_version_t version, qr_errorlevel_t level, qr_maskpattern_t mask, const char *src);

void qrmatrix_free(qrmatrix_t *qr);

// --------------------------------------

bit_t qrmatrix_set_maskpattern(qrmatrix_t *qr, qr_maskpattern_t mask);
qr_maskpattern_t qrmatrix_get_maskpattern(qrmatrix_t *qr);

bit_t qrmatrix_set_errorlevel(qrmatrix_t *qr, qr_errorlevel_t level);
qr_errorlevel_t qrmatrix_get_errorlevel(qrmatrix_t *qr);

bit_t qrmatrix_set_version(qrmatrix_t *qr, qr_version_t v);
qr_version_t qrmatrix_get_version(qrmatrix_t *qr);

bit_t qrmatrix_set_version_info(qrmatrix_t *qr, qrversion_t vi);
bit_t qrmatrix_set_format_info(qrmatrix_t *qr, qrformat_t fi);

bit_t qrmatrix_set_padding(qrmatrix_t *qr, padding_t padding);
padding_t qrmatrix_get_padding(qrmatrix_t *qr);

uint_fast8_t qrmatrix_get_width(qrmatrix_t *qr);
uint_fast8_t qrmatrix_get_height(qrmatrix_t *qr);
// --------------------------------------

int qrmatrix_fix_errors(qrmatrix_t *qr);

unsigned int qrmatrix_score(qrmatrix_t *qr);

void qrmatrix_dump(qrmatrix_t *qr, FILE *out);

void qrmatrix_fill(qrmatrix_t *qr, bit_t v);

// --------------------------------------

size_t qrmatrix_write_string(qrmatrix_t *qr, const char *src);
size_t qrmatrix_read_string(qrmatrix_t *qr, char *buffer, size_t size);

size_t qrmatrix_write_string_numeric(qrmatrix_t *qr, const char *src);
size_t qrmatrix_write_string_alnum(qrmatrix_t *qr, const char *src);
size_t qrmatrix_write_string_8bit(qrmatrix_t *qr, const char *src);

size_t qrmatrix_write_buffer(qrmatrix_t *qr, const char *src, size_t len);
size_t qrmatrix_write_buffer_numeric(qrmatrix_t *qr, const char *src, size_t len);
size_t qrmatrix_write_buffer_alnum(qrmatrix_t *qr, const char *src, size_t len);
size_t qrmatrix_write_buffer_8bit(qrmatrix_t *qr, const char *src, size_t len);

// --------------------------------------
void qrmatrix_write_pixel(qrmatrix_t *qr, int x, int y, bit_t v);
bit_t qrmatrix_read_pixel(qrmatrix_t *qr, int x, int y);

size_t qrmatrix_write_bitmap(qrmatrix_t *qr, const void *src, size_t len, bitpos_t bpp);
size_t qrmatrix_read_bitmap(qrmatrix_t *qr, void *src, size_t len, bitpos_t bpp);

void qrmatrix_write_finder_pattern(qrmatrix_t *qr);
int qrmatrix_read_finder_pattern(qrmatrix_t *qr, int idx);

void qrmatrix_write_alignment_pattern(qrmatrix_t *qr);
int qrmatrix_read_alignment_pattern(qrmatrix_t *qr, int idx);

void qrmatrix_write_timing_pattern(qrmatrix_t *qr);
int qrmatrix_read_timing_pattern(qrmatrix_t *qr);

void qrmatrix_write_format_info(qrmatrix_t *qr);
qrformat_t qrmatrix_read_format_info(qrmatrix_t *qr);

void qrmatrix_write_version_info(qrmatrix_t *qr);
qrversion_t qrmatrix_read_version_info(qrmatrix_t *qr);
qr_version_t qrmatrix_read_version(qrmatrix_t *qr);

bitpos_t qrmatrix_write_payload(qrmatrix_t *qr, qrpayload_t *payload);
bitpos_t qrmatrix_read_payload(qrmatrix_t *qr, qrpayload_t *payload);

void qrmatrix_write_function_patterns(qrmatrix_t *qr);
bitpos_t qrmatrix_write_all(qrmatrix_t *qr, qrpayload_t *payload);

// --------------------------------------

bitstream_t qrmatrix_create_bitstream(qrmatrix_t *qr, bitstream_iterator_t iter);
bitstream_t qrmatrix_create_bitstream_for_composed_data(qrmatrix_t *qr);
bitstream_t qrmatrix_create_bitstream_for_alignment_pattern(qrmatrix_t *qr, int idx);

// --------------------------------------

#ifndef NO_CALLBACK
void qrmatrix_on_write_pixel(qrmatrix_t *qr,
                             bit_t (*write_pixel)(qrmatrix_t *qr, bitpos_t x, bitpos_t y, bitpos_t pos, bit_t v, void *opaque),
                             void *opaque);
void qrmatrix_on_read_pixel(qrmatrix_t *qr, bit_t (*read_pixel)(qrmatrix_t *qr, bitpos_t x, bitpos_t y, bitpos_t pos, void *opaque),
                            void *opaque);
#endif

#endif /* __QR_MATRIX_H__ */
