#ifndef __QR_QREAN_H__
#define __QR_QREAN_H__

#include <stdint.h>
#include <stdio.h>

#include "qrformat.h"
#include "qrpayload.h"
#include "qrtypes.h"
#include "qrversion.h"
#include "utils.h"

// Max to QR version 40
#define QREAN_CANVAS_MAX_BUFFER_SIZE (3917)

typedef struct _qrean_canvas_t qrean_canvas_t;
typedef struct _qrean_t qrean_t;

typedef enum {
	QREAN_CODE_TYPE_INVALID,

	QREAN_CODE_TYPE_QR,
	QREAN_CODE_TYPE_MQR,
	QREAN_CODE_TYPE_RMQR,

	QREAN_CODE_TYPE_EAN13,
	QREAN_CODE_TYPE_EAN8,
	QREAN_CODE_TYPE_UPCA,
	QREAN_CODE_TYPE_CODE39,
	QREAN_CODE_TYPE_CODE93,
	QREAN_CODE_TYPE_NW7,
	QREAN_CODE_TYPE_ITF,
} qrean_code_type_t;

typedef enum {
	QREAN_DATA_TYPE_AUTO,
	QREAN_DATA_TYPE_NUMERIC,
	QREAN_DATA_TYPE_ALNUM,
	QREAN_DATA_TYPE_8BIT,
	QREAN_DATA_TYPE_KANJI,
} qrean_data_type_t;

typedef struct {
	bitstream_iterator_t iter;
	const uint8_t *bits;
	bitpos_t size;
} qrean_bitpattern_t;

typedef struct {
	qrean_code_type_t type;

	void (*init)(qrean_t *qrean);
	void (*deinit)(qrean_t *qrean);

	bitpos_t (*write_data)(qrean_t *qrean, const void *buf, size_t len, qrean_data_type_t data_type);
	bitpos_t (*read_data)(qrean_t *qrean, void *buf, size_t size);

	unsigned int (*score)(qrean_t *qrean);

	bitstream_iterator_t data_iter;

	// QR related bitpatterns
	union {
		struct {
			qrean_bitpattern_t finder_pattern;
			qrean_bitpattern_t finder_sub_pattern;
			qrean_bitpattern_t corner_finder_pattern;
			qrean_bitpattern_t timing_pattern;
			qrean_bitpattern_t alignment_pattern;
			qrean_bitpattern_t format_info;
			qrean_bitpattern_t version_info;
		} qr;
	};
} qrean_code_t;

#define QREAN_IS_TYPE_QRFAMILY(qrean) (QREAN_CODE_TYPE_QR <= (qrean)->code->type && (qrean)->code->type <= QREAN_CODE_TYPE_RMQR)
#define QREAN_IS_TYPE_BARCODE(qrean)  (QREAN_CODE_TYPE_EAN13 <= (qrean)->code->type && (qrean)->code->type <= QREAN_CODE_TYPE_ITF)
#define QREAN_IS_TYPE_QR(qrean)       ((qrean)->code->type == QREAN_CODE_TYPE_QR)
#define QREAN_IS_TYPE_MQR(qrean)      ((qrean)->code->type == QREAN_CODE_TYPE_MQR)
#define QREAN_IS_TYPE_RMQR(qrean)     ((qrean)->code->type == QREAN_CODE_TYPE_RMQR)

struct _qrean_canvas_t {
	uint8_t symbol_width;
	uint8_t symbol_height;

	uint8_t stride;

	// for bitmap read/write
	uint8_t bitmap_width;
	uint8_t bitmap_height;
	uint8_t bitmap_scale;
	padding_t bitmap_padding;

#if defined(USE_MALLOC_BUFFER) && defined(NO_MALLOC)
#error "Specify both of USE_MALLOC_BUFFER and NO_MALLOC doesn't make sense"
#endif
#ifdef USE_MALLOC_BUFFER
	uint8_t *buffer;
#else
	uint8_t buffer[QREAN_CANVAS_MAX_BUFFER_SIZE];
#endif

#ifndef NO_CALLBACK
	bit_t (*write_pixel)(qrean_t *qrean, bitpos_t x, bitpos_t y, bitpos_t pos, bit_t v, void *opaque);
	void *opaque_write;
	bit_t (*read_pixel)(qrean_t *qrean, bitpos_t x, bitpos_t y, bitpos_t pos, void *opaque);
	void *opaque_read;
#endif
};

struct _qrean_t {
	const qrean_code_t *code;
	qrean_canvas_t canvas;

	union {
		struct {
			qr_version_t version;
			qr_errorlevel_t level;
			qr_maskpattern_t mask;
		} qr;
	};
};

// ========= creation / free
bit_t qrean_init(qrean_t *qrean, qrean_code_type_t type);
qrean_t create_qrean(qrean_code_type_t type);
void qrean_destroy(qrean_t *qrean);
qrean_t *new_qrean(qrean_code_type_t type);
void qrean_free(qrean_t *qrean);

// ========= code operation (QR)
bit_t qrean_set_symbol_size(qrean_t *qrean, uint8_t width, uint8_t height);
bit_t qrean_set_symbol_width(qrean_t *qrean, uint8_t width);
bit_t qrean_set_symbol_height(qrean_t *qrean, uint8_t width);

bit_t qrean_set_qr_version(qrean_t *qrean, qr_version_t version);
bit_t qrean_set_qr_errorlevel(qrean_t *qrean, qr_errorlevel_t level);
bit_t qrean_set_qr_maskpattern(qrean_t *qrean, qr_maskpattern_t mask);
bit_t qrean_set_qr_format_info(qrean_t *qrean, qrformat_t fi);
bit_t qrean_set_qr_version_info(qrean_t *qrean, qrversion_t vi);

// ========= code operation (Barcode)
bit_t qrean_set_barcode_height(uint8_t height);
bit_t qrean_get_barcode_height(uint8_t height);
// TODO: qrean_set_barcode_XXX

// ========= code operation (Common)
int qrean_fix_errors(qrean_t *qrean);
int qrean_check_errors(qrean_t *qrean);

// ========= code interaction (read/write)
qrformat_t qrean_read_qr_format_info(qrean_t *qrean, int idx);
qrversion_t qrean_read_qr_version_info(qrean_t *qrean, int idx);
int qrean_read_qr_finder_pattern(qrean_t *qrean, int idx);
int qrean_read_qr_finder_sub_pattern(qrean_t *qrean, int idx);
int qrean_read_qr_corner_finder_pattern(qrean_t *qrean, int idx);
int qrean_read_qr_timing_pattern(qrean_t *qrean, int idx);
int qrean_read_qr_alignment_pattern(qrean_t *qrean, int idx);

qr_version_t qrean_read_qr_version(qrean_t *qrean);
qr_maskpattern_t qrean_read_qr_maskpattern(qrean_t *qrean);
qr_errorlevel_t qrean_read_qr_errorlevel(qrean_t *qrean);

void qrean_write_qr_format_info(qrean_t *qrean);
void qrean_write_qr_version_info(qrean_t *qrean);
void qrean_write_qr_finder_pattern(qrean_t *qrean);
void qrean_write_qr_finder_sub_pattern(qrean_t *qrean);
void qrean_write_qr_corner_finder_pattern(qrean_t *qrean);
void qrean_write_qr_timing_pattern(qrean_t *qrean);
void qrean_write_qr_alignment_pattern(qrean_t *qrean);

size_t qrean_write_qr_data(qrean_t *qrean, const void *buffer, size_t len, qrean_data_type_t data_type);
size_t qrean_read_qr_data(qrean_t *qrean, void *buffer, size_t size);

bitpos_t qrean_write_qr_payload(qrean_t *qrean, qrpayload_t *payload);
bitpos_t qrean_read_qr_payload(qrean_t *qrean, qrpayload_t *payload);

void qrean_write_frame(qrean_t *qrean);

size_t qrean_write_string(qrean_t *qrean, const char *str, qrean_data_type_t data_type);
size_t qrean_write_buffer(qrean_t *qrean, const char *buffer, size_t len, qrean_data_type_t data_type);
size_t qrean_read_string(qrean_t *qrean, char *buffer, size_t size);
size_t qrean_read_buffer(qrean_t *qrean, char *buffer, size_t size);

void qrean_write_pixel(qrean_t *qrean, int x, int y, bit_t v);
bit_t qrean_read_pixel(qrean_t *qrean, int x, int y);

// void qrean_write_payload(qrean_t *qrean, void *payload);
// void qrean_read_payload(qrean_t *qrean, void *payload);

size_t qrean_write_bitmap(qrean_t *qrean, const void *buffer, size_t size, bitpos_t bpp);
size_t qrean_read_bitmap(qrean_t *qrean, void *buffer, size_t size, bitpos_t bpp);

bitpos_t qrean_read_bitstream(qrean_t *qrean, bitstream_t src);
bitpos_t qrean_write_bitstream(qrean_t *qrean, bitstream_t src);

// ========= canvas
bit_t qrean_set_bitmap_padding(qrean_t *qrean, padding_t padding);
padding_t qrean_get_bitmap_padding(qrean_t *qrean);

bit_t qrean_set_bitmap_scale(qrean_t *qrean, uint8_t scale);
uint8_t qrean_get_bitmap_scale(qrean_t *qrean);

size_t qrean_get_bitmap_width(qrean_t *qrean);
size_t qrean_get_bitmap_height(qrean_t *qrean);

void qrean_dump(qrean_t *qrean, FILE *out);

void qrean_fill(qrean_t *qrean, bit_t v);

// canvas callback
void qrean_on_write_pixel(
	qrean_t *qrean, bit_t (*write_pixel)(qrean_t *qrean, bitpos_t x, bitpos_t y, bitpos_t pos, bit_t v, void *opaque), void *opaque);
void qrean_on_read_pixel(
	qrean_t *qrean, bit_t (*read_pixel)(qrean_t *qrean, bitpos_t x, bitpos_t y, bitpos_t pos, void *opaque), void *opaque);

// misc
bitstream_t qrean_create_bitstream(qrean_t *qrean, bitstream_iterator_t iter);

int qrean_check_qr_combination(qrean_t *qrean);

#endif /* __QR_QREAN_H__ */
