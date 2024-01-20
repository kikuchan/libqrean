#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitstream.h"
#include "codes.h"
#include "debug.h"
#include "qrdata.h"
#include "qrean.h"
#include "qrformat.h"
#include "qrpayload.h"
#include "qrspec.h"
#include "qrversion.h"
#include "runlength.h"

bit_t qrean_init(qrean_t *qrean, qrean_code_type_t type)
{
	for (size_t i = 0; i < sizeof(codes) / sizeof(codes[0]); i++) {
		if (codes[i]->type == type) {
			// FOUND
			qrean->code = codes[i];

			if (QREAN_IS_TYPE_QRFAMILY(qrean)) {
				qrean_set_qr_version(qrean, QR_VERSION_AUTO);
				qrean_set_qr_errorlevel(qrean, QR_ERRORLEVEL_M);
				qrean_set_qr_maskpattern(qrean, QR_MASKPATTERN_AUTO);
				qrean_set_eci_code(qrean, QR_ECI_CODE_LATIN1);
				qrean->canvas.stride = 177;
			} else {
				qrean_set_symbol_height(qrean, 10);
				qrean->canvas.stride = 0;
			}
			qrean_set_bitmap_padding(qrean, qrean->code->padding);

			qrean_set_bitmap_scale(qrean, 4);
			qrean_set_bitmap_color(qrean, 0x00000000, 0xffffffff);

#ifndef NO_CANVAS_BUFFER
#if defined(USE_MALLOC_BUFFER) && !defined(NO_MALLOC)
			qrean->canvas.buffer = (uint8_t *)malloc(QREAN_CANVAS_MAX_BUFFER_SIZE);
#endif
			memset(qrean->canvas.buffer, 0, QREAN_CANVAS_MAX_BUFFER_SIZE);
#endif

			if (qrean->code->init) qrean->code->init(qrean);

			return 1;
		}
	}
	return 0;
}

void qrean_deinit(qrean_t *qrean)
{
	if (qrean->code && qrean->code->deinit) qrean->code->deinit(qrean);
#ifndef NO_CANVAS_BUFFER
#ifdef USE_MALLOC_BUFFER
	free(qrean->canvas.buffer);
#endif
#endif
}

qrean_t create_qrean(qrean_code_type_t type)
{
	qrean_t qrean = {};
	qrean_init(&qrean, type);
	return qrean;
}

void qrean_destroy(qrean_t *qrean)
{
	qrean_deinit(qrean);
}

qrean_t *new_qrean(qrean_code_type_t type)
{
#ifndef NO_MALLOC
	qrean_t *qrean = (qrean_t *)calloc(1, sizeof(qrean_t));
	if (!qrean) return NULL;
	if (!qrean_init(qrean, type)) {
		free(qrean);
		return NULL;
	}
	return qrean;
#else
	return NULL;
#endif
}

void qrean_free(qrean_t *qrean)
{
#ifndef NO_MALLOC
	qrean_deinit(qrean);
	free(qrean);
#endif
}

bit_t qrean_set_qr_version(qrean_t *qrean, qr_version_t version)
{
	if (!QREAN_IS_TYPE_QRFAMILY(qrean)) return 0;

	if (version == QR_VERSION_INVALID) {
		qrean_error("Invalid version");
		return 0;
	}
	qrean->qr.version = version;
	qrean_set_symbol_size(qrean, qrspec_get_symbol_width(version), qrspec_get_symbol_height(version));
	return 1;
}

bit_t qrean_set_qr_maskpattern(qrean_t *qrean, qr_maskpattern_t mask)
{
	if (!QREAN_IS_TYPE_QRFAMILY(qrean)) return 0;

	if (mask == QR_MASKPATTERN_INVALID) {
		qrean_error("Invalid maskpattern");
		return 0;
	}
	qrean->qr.mask = mask;
	return 1;
}

bit_t qrean_set_qr_errorlevel(qrean_t *qrean, qr_errorlevel_t level)
{
	if (!QREAN_IS_TYPE_QRFAMILY(qrean)) return 0;

	if (level == QR_ERRORLEVEL_INVALID) return 0;
	qrean->qr.level = level;
	return 1;
}

bit_t qrean_set_symbol_size(qrean_t *qrean, uint8_t width, uint8_t height)
{
	qrean->canvas.symbol_width = width;
	qrean->canvas.symbol_height = height;
	qrean->canvas.bitmap_width = qrean->canvas.bitmap_padding.l + qrean->canvas.symbol_width + qrean->canvas.bitmap_padding.r;
	qrean->canvas.bitmap_height = qrean->canvas.bitmap_padding.t + qrean->canvas.symbol_height + qrean->canvas.bitmap_padding.b;
	return 1;
}

bit_t qrean_set_symbol_width(qrean_t *qrean, uint8_t width)
{
	qrean->canvas.symbol_width = width;
	qrean->canvas.bitmap_width = qrean->canvas.bitmap_padding.l + qrean->canvas.symbol_width + qrean->canvas.bitmap_padding.r;
	return 1;
}

bit_t qrean_set_symbol_height(qrean_t *qrean, uint8_t height)
{
	qrean->canvas.symbol_height = height;
	qrean->canvas.bitmap_height = qrean->canvas.bitmap_padding.t + qrean->canvas.symbol_height + qrean->canvas.bitmap_padding.b;
	return 1;
}

uint8_t qrean_get_symbol_width(qrean_t *qrean)
{
	return qrean->canvas.symbol_width;
}

uint8_t qrean_get_symbol_height(qrean_t *qrean)
{
	return qrean->canvas.symbol_height;
}

bit_t qrean_set_bitmap_padding(qrean_t *qrean, padding_t padding)
{
	// TODO:
	qrean->canvas.bitmap_padding = padding;
	qrean->canvas.bitmap_width = qrean->canvas.bitmap_padding.l + qrean->canvas.symbol_width + qrean->canvas.bitmap_padding.r;
	qrean->canvas.bitmap_height = qrean->canvas.bitmap_padding.t + qrean->canvas.symbol_height + qrean->canvas.bitmap_padding.b;
	return 1;
}

padding_t qrean_get_bitmap_padding(qrean_t *qrean)
{
	return qrean->canvas.bitmap_padding;
}

bit_t qrean_set_bitmap_scale(qrean_t *qrean, uint8_t scale)
{
	qrean->canvas.bitmap_scale = scale;
	return 1;
}

bit_t qrean_set_bitmap_color(qrean_t *qrean, uint32_t dark, uint32_t light)
{
	qrean->canvas.bitmap_color_dark = dark;
	qrean->canvas.bitmap_color_light = light;
	return 1;
}

uint8_t qrean_get_bitmap_scale(qrean_t *qrean)
{
	return qrean->canvas.bitmap_scale;
}

size_t qrean_get_bitmap_width(qrean_t *qrean)
{
	return qrean->canvas.bitmap_width * qrean->canvas.bitmap_scale;
}

size_t qrean_get_bitmap_height(qrean_t *qrean)
{
	return qrean->canvas.bitmap_height * qrean->canvas.bitmap_scale;
}

void qrean_fill(qrean_t *qrean, bit_t v)
{
	bitstream_t bs = qrean_create_bitstream(qrean, NULL);
	bitstream_fill(&bs, v);
}

static void qrean_write_bit_at(bitstream_t *bs, bitpos_t pos, void *opaque, bit_t v)
{
	qrean_t *qrean = (qrean_t *)opaque;
#ifndef NO_CALLBACK
	if (qrean->canvas.write_pixel) {
		bitpos_t x = !qrean->canvas.stride ? pos : (pos % qrean->canvas.stride);
		bitpos_t y = !qrean->canvas.stride ? 0 : (pos / qrean->canvas.stride);

		if (qrean->canvas.write_pixel(qrean, x, y, pos, v, qrean->canvas.opaque_write)) return;
	}
#endif

#ifndef NO_CANVAS_BUFFER
	WRITE_BIT(qrean->canvas.buffer, pos, v);
#endif
}

static bit_t qrean_read_bit_at(bitstream_t *bs, bitpos_t pos, void *opaque)
{
	qrean_t *qrean = (qrean_t *)opaque;
#ifndef NO_CALLBACK
	if (qrean->canvas.read_pixel) {
		bitpos_t x = !qrean->canvas.stride ? pos : (pos % qrean->canvas.stride);
		bitpos_t y = !qrean->canvas.stride ? 0 : (pos / qrean->canvas.stride);
		return qrean->canvas.read_pixel(qrean, x, y, pos, qrean->canvas.opaque_read);
	} else
#endif
	{
#ifndef NO_CANVAS_BUFFER
		return READ_BIT(qrean->canvas.buffer, pos);
#else
		return 0;
#endif
	}
}

void qrean_write_pixel(qrean_t *qrean, int x, int y, bit_t v)
{
	if (x < 0) return;
	if (y < 0) return;
	if (x >= qrean->canvas.symbol_width) return;
	if (y >= qrean->canvas.symbol_height) return;

	bitpos_t pos = QREAN_XY_TO_BITPOS(qrean, x, y);
	qrean_write_bit_at(NULL, pos, qrean, v);
}

bit_t qrean_read_pixel(qrean_t *qrean, int x, int y)
{
	if (x < 0) return 0;
	if (y < 0) return 0;
	if (x >= qrean->canvas.symbol_width) return 0;
	if (y >= qrean->canvas.symbol_height) return 0;

	bitpos_t pos = QREAN_XY_TO_BITPOS(qrean, x, y);
	return qrean_read_bit_at(NULL, pos, qrean);
}

void qrean_dump(qrean_t *qrean, FILE *out)
{
#ifndef NO_PRINTF
	const char *dots[4] = { "\u2588", "\u2580", "\u2584", " " };

	int_fast16_t sx = -qrean->canvas.bitmap_padding.l;
	int_fast16_t sy = -qrean->canvas.bitmap_padding.t;
	int_fast16_t ex = (int_fast16_t)qrean->canvas.symbol_width + qrean->canvas.bitmap_padding.r;
	int_fast16_t ey = (int_fast16_t)qrean->canvas.symbol_height + qrean->canvas.bitmap_padding.b;
	for (int_fast16_t y = sy; y < ey; y += 2) {
		for (int_fast16_t x = sx; x < ex; x++) {
			bit_t u = qrean_read_pixel(qrean, x, y + 0);
			bit_t l = y + 1 >= ey ? 1 : qrean_read_pixel(qrean, x, y + 1);

			fprintf(out, "%s", dots[((u << 1) | l) & 3]);
		}
		fprintf(out, "\n");
	}
#endif
}

bitstream_t qrean_create_bitstream(qrean_t *qrean, bitstream_iterator_t iter)
{
	bitstream_t bs = create_bitstream(qrean->canvas.buffer, QREAN_CANVAS_MAX_BUFFER_SIZE * 8, iter, qrean);
	bitstream_on_write_bit(&bs, qrean_write_bit_at, qrean);
	bitstream_on_read_bit(&bs, qrean_read_bit_at, qrean);
	return bs;
}

int qrean_fix_errors(qrean_t *qrean)
{
	qrpayload_t payload = {};
	qrpayload_init(&payload, qrean->qr.version, qrean->qr.level);
	qrean_read_qr_payload(qrean, &payload);

	int n = qrpayload_fix_errors(&payload);
	if (n > 0) {
		qrean_write_qr_payload(qrean, &payload);
	}

	qrpayload_deinit(&payload);

	return n;
}

#ifndef NO_CALLBACK
void qrean_on_write_pixel(
	qrean_t *qrean, bit_t (*write_pixel)(qrean_t *qrean, bitpos_t x, bitpos_t y, bitpos_t pos, bit_t v, void *opaque), void *opaque)
{
	qrean->canvas.write_pixel = write_pixel;
	qrean->canvas.opaque_write = opaque;
}

void qrean_on_read_pixel(
	qrean_t *qrean, bit_t (*read_pixel)(qrean_t *qrean, bitpos_t x, bitpos_t y, bitpos_t pos, void *opaque), void *opaque)
{
	qrean->canvas.read_pixel = read_pixel;
	qrean->canvas.opaque_read = opaque;
}
#endif

size_t qrean_read_string(qrean_t *qrean, char *buffer, size_t size)
{
	size_t len = qrean_read_buffer(qrean, buffer, size);
	if (len >= size) len = size - 1;
	buffer[len] = '\0';

	return len;
}

static bitpos_t bitmap_iter(bitstream_t *bs, bitpos_t i, void *opaque)
{
	qrean_t *qrean = (qrean_t *)opaque;

	if (i >= qrean->canvas.bitmap_width * qrean->canvas.bitmap_height * qrean->canvas.bitmap_scale * qrean->canvas.bitmap_scale)
		return BITPOS_END;

	int_fast32_t x = (int_fast32_t)(i / qrean->canvas.bitmap_scale % qrean->canvas.bitmap_width) - qrean->canvas.bitmap_padding.l;
	int_fast32_t y = (int_fast32_t)(i / qrean->canvas.bitmap_scale / qrean->canvas.bitmap_width / qrean->canvas.bitmap_scale)
		- qrean->canvas.bitmap_padding.t;

	return QREAN_XYV_TO_BITPOS(qrean, x, y, 0);
}

size_t qrean_read_bitmap(qrean_t *qrean, void *buffer, size_t size, bitpos_t bpp)
{
	bitstream_t src = qrean_create_bitstream(qrean, bitmap_iter);
	bitstream_t dst = create_bitstream(buffer, size * 8, NULL, NULL);

	while (!bitstream_is_end(&src) && !bitstream_is_end(&dst)) {
		uint32_t v = bitstream_read_bit(&src) ? qrean->canvas.bitmap_color_dark : qrean->canvas.bitmap_color_light;
		bitstream_write_bits(&dst, v, bpp);
	}
	return bitstream_tell(&dst) / 8;
}

size_t qrean_write_bitmap(qrean_t *qrean, const void *buffer, size_t size, bitpos_t bpp)
{
	bitstream_t dst = qrean_create_bitstream(qrean, bitmap_iter);
	bitstream_t src = create_bitstream(buffer, size * 8, NULL, NULL);

	while (!bitstream_is_end(&src) && !bitstream_is_end(&dst)) {
		bit_t v = bitstream_read_bits(&src, bpp) ? 0 : 1;
		bitstream_write_bit(&dst, v);
	}
	return bitstream_tell(&src) / 8;
}

bitpos_t qrean_write_bitstream(qrean_t *qrean, bitstream_t src)
{
	bitstream_t dst = qrean_create_bitstream(qrean, qrean->code->data_iter);
	return bitstream_copy(&dst, &src, 0, 0);
}

bitpos_t qrean_read_bitstream(qrean_t *qrean, bitstream_t dst)
{
	bitstream_t src = qrean_create_bitstream(qrean, qrean->code->data_iter);
	return bitstream_copy(&dst, &src, 0, 0);
}

size_t qrean_write_string(qrean_t *qrean, const char *str, qrean_data_type_t data_type)
{
	return qrean_write_buffer(qrean, str, strlen(str), data_type);
}

// QR frames

void qrean_write_qr_format_info(qrean_t *qrean)
{
	if (!qrean->code->qr.format_info.iter) return;
	bitstream_t bs = qrean_create_bitstream(qrean, qrean->code->qr.format_info.iter);
	qrformat_t fi = qrformat_for(qrean->qr.version, qrean->qr.level, qrean->qr.mask);

	bitstream_write_bits(&bs, fi.value, qrean->code->qr.format_info.size);
	bitstream_write_bits(&bs, fi.value, qrean->code->qr.format_info.size);

	// XXX: dark module
	if (QREAN_IS_TYPE_QR(qrean)) {
		qrean_write_pixel(qrean, 8, qrean->canvas.symbol_height - 8, 1);
	}
}

qrformat_t qrean_read_qr_format_info(qrean_t *qrean, int idx)
{
	bitstream_t bs = qrean_create_bitstream(qrean, qrean->code->qr.format_info.iter);

	qrformat_t fi1 = qrformat_from(qrean->qr.version, bitstream_read_bits(&bs, qrean->code->qr.format_info.size));
	qrformat_t fi2 = qrformat_from(qrean->qr.version, bitstream_read_bits(&bs, qrean->code->qr.format_info.size));

	if (idx != 1 && (idx == 0 || fi1.mask != QR_MASKPATTERN_INVALID)) return fi1;
	return fi2;
}

bit_t qrean_set_qr_format_info(qrean_t *qrean, qrformat_t fi)
{
	if (fi.level == QR_ERRORLEVEL_INVALID || fi.mask == QR_MASKPATTERN_INVALID) return 0;

	if (QREAN_IS_TYPE_RMQR(qrean) || QREAN_IS_TYPE_MQR(qrean)) {
		qrean_set_qr_version(qrean, fi.version);
	}
	qrean_set_qr_errorlevel(qrean, fi.level);
	qrean_set_qr_maskpattern(qrean, fi.mask);
	return 1;
}

bit_t qrean_set_qr_version_info(qrean_t *qrean, qrversion_t vi)
{
	return qrean_set_qr_version(qrean, vi.version);
}

void qrean_write_qr_version_info(qrean_t *qrean)
{
	if (!qrean->code->qr.version_info.iter) return;
	bitstream_t bs = qrean_create_bitstream(qrean, qrean->code->qr.version_info.iter);
	qrversion_t vi = qrversion_for(qrean->qr.version);

	bitstream_write_bits(&bs, vi.value, qrean->code->qr.version_info.size);
	bitstream_write_bits(&bs, vi.value, qrean->code->qr.version_info.size);
}

qrversion_t qrean_read_qr_version_info(qrean_t *qrean, int idx)
{
	if (QREAN_IS_TYPE_RMQR(qrean) || QREAN_IS_TYPE_MQR(qrean)) {
		// XXX: dummy
		qrformat_t fi = qrean_read_qr_format_info(qrean, idx);
		return qrversion_for(fi.version);
	}
	if (qrean->qr.version < QR_VERSION_7) {
		// XXX: dummy
		return qrversion_for(qrean->qr.version);
	}

	bitstream_t bs = qrean_create_bitstream(qrean, qrean->code->qr.version_info.iter);

	qrversion_t vi1 = qrversion_from(bitstream_read_bits(&bs, qrean->code->qr.version_info.size));
	qrversion_t vi2 = qrversion_from(bitstream_read_bits(&bs, qrean->code->qr.version_info.size));

	if (idx != 1 && (idx == 0 || vi1.version != QR_VERSION_INVALID)) return vi1;
	return vi2;
}

qr_version_t qrean_read_qr_version(qrean_t *qrean)
{
	qrversion_t v = qrean_read_qr_version_info(qrean, -1);
	return v.version;
}

qr_maskpattern_t qrean_read_qr_maskpattern(qrean_t *qrean)
{
	qrformat_t fi = qrean_read_qr_format_info(qrean, -1);
	return fi.mask;
}

qr_errorlevel_t qrean_read_qr_errorlevel(qrean_t *qrean)
{
	qrformat_t fi = qrean_read_qr_format_info(qrean, -1);
	return fi.level;
}

#define DEFINE_QR_PATTERN(name)                                                                               \
	void qrean_write_qr_##name(qrean_t *qrean)                                                                \
	{                                                                                                         \
		if (!qrean->code->qr.name.bits) return;                                                               \
		bitstream_t bs = qrean_create_bitstream(qrean, qrean->code->qr.name.iter);                            \
		bitstream_write(&bs, qrean->code->qr.name.bits, qrean->code->qr.name.size, 0);                        \
	}                                                                                                         \
                                                                                                              \
	int qrean_read_qr_##name(qrean_t *qrean, int idx)                                                         \
	{                                                                                                         \
		if (!qrean->code->qr.name.bits) return 0;                                                             \
		bitstream_t bs = qrean_create_bitstream(qrean, qrean->code->qr.name.iter);                            \
		return calc_pattern_mismatch_error_rate(                                                              \
			&bs, qrean->code->qr.name.bits, qrean->code->qr.name.size, idx >= 0 ? idx : 0, idx >= 0 ? 1 : 0); \
	}

DEFINE_QR_PATTERN(finder_pattern);
DEFINE_QR_PATTERN(finder_sub_pattern);
DEFINE_QR_PATTERN(corner_finder_pattern);
DEFINE_QR_PATTERN(border_pattern);
DEFINE_QR_PATTERN(alignment_pattern);
DEFINE_QR_PATTERN(timing_pattern);

bitpos_t qrean_write_qr_payload(qrean_t *qrean, qrpayload_t *payload)
{
	return qrean_write_bitstream(qrean, qrpayload_get_bitstream(payload));
}

bitpos_t qrean_read_qr_payload(qrean_t *qrean, qrpayload_t *payload)
{
	return qrean_read_bitstream(qrean, qrpayload_get_bitstream(payload));
}

void qrean_write_frame(qrean_t *qrean)
{
	if (QREAN_IS_TYPE_QRFAMILY(qrean)) {
		qrean_write_qr_finder_pattern(qrean);
		qrean_write_qr_finder_sub_pattern(qrean);
		qrean_write_qr_corner_finder_pattern(qrean);
		qrean_write_qr_border_pattern(qrean);
		qrean_write_qr_alignment_pattern(qrean);
		qrean_write_qr_timing_pattern(qrean);
		qrean_write_qr_format_info(qrean);
		qrean_write_qr_version_info(qrean);
	} else if (QREAN_IS_TYPE_BARCODE(qrean)) {
		// TODO
	}
}
unsigned int qrean_read_qr_score(qrean_t *qrean)
{
	if (qrean->code->score) return qrean->code->score(qrean);
	return 0;
}

static size_t qrean_try_write_qr_data(qrean_t *qrean, const void *buffer, size_t len, qrean_data_type_t data_type)
{
	qrpayload_t payload = {};
	qrpayload_init(&payload, qrean->qr.version, qrean->qr.level);
	size_t retval = 0;

	qrdata_writer_t writer = qrdata_write_8bit_string;
	switch (data_type) {
	case QREAN_DATA_TYPE_AUTO:
		writer = qrdata_write_string;
		break;
	case QREAN_DATA_TYPE_NUMERIC:
		writer = qrdata_write_numeric_string;
		break;
	case QREAN_DATA_TYPE_ALNUM:
		writer = qrdata_write_alnum_string;
		break;
	case QREAN_DATA_TYPE_8BIT:
		writer = qrdata_write_8bit_string;
		break;
	case QREAN_DATA_TYPE_KANJI:
		writer = qrdata_write_kanji_string;
		break;
	}

	if (writer && qrpayload_write_string(&payload, (const char *)buffer, len, writer, qrean->eci_code)) {
		qr_maskpattern_t min_mask = qrean->qr.mask;
		if (qrean->qr.mask == QR_MASKPATTERN_AUTO) {
			unsigned int min_score = UINT_MAX;
			min_mask = QR_MASKPATTERN_0; // just in case

			for (uint_fast8_t m = QR_MASKPATTERN_0; m <= QR_MASKPATTERN_7; m++) {
				if (!qrspec_is_valid_combination(qrean->qr.version, qrean->qr.level, (qr_maskpattern_t)m)) continue;

				qrean_set_qr_maskpattern(qrean, (qr_maskpattern_t)m);
				qrean_write_frame(qrean);
				qrean_write_qr_payload(qrean, &payload);

				unsigned int score = qrean_read_qr_score(qrean);
				if (min_score > score) {
					min_score = score;
					min_mask = (qr_maskpattern_t)m;
				}
			}
		}
		qrean_set_qr_maskpattern(qrean, min_mask);
		qrean_write_frame(qrean);
		retval = qrean_write_qr_payload(qrean, &payload);
	}
	qrpayload_deinit(&payload);
	return retval;
}

// Write actual data and finalize.
// This function may alter QR version and/or QR mask pattern settings in qrean
size_t qrean_write_qr_data(qrean_t *qrean, const void *buffer, size_t len, qrean_data_type_t data_type)
{
	int min_v, max_v;
	if (qrean->qr.version == QR_VERSION_AUTO) {
		min_v = QREAN_IS_TYPE_QR(qrean) ? QR_VERSION_1
			: QREAN_IS_TYPE_MQR(qrean)  ? QR_VERSION_M1
			: QREAN_IS_TYPE_TQR(qrean)  ? QR_VERSION_TQR
										: QR_VERSION_R7x43;
		max_v = QREAN_IS_TYPE_QR(qrean) ? QR_VERSION_40
			: QREAN_IS_TYPE_MQR(qrean)  ? QR_VERSION_M4
			: QREAN_IS_TYPE_TQR(qrean)  ? QR_VERSION_TQR
										: QR_VERSION_R17x139;
	} else {
		min_v = max_v = qrean->qr.version;
	}

	qr_maskpattern_t mask = qrean->qr.mask;
	for (int v = min_v; v <= max_v; v++) {
		if (!qrspec_is_valid_combination((qr_version_t)v, qrean->qr.level, mask)) continue;

		qrean_set_qr_version(qrean, (qr_version_t)v);
		qrean_set_qr_maskpattern(qrean, mask);
		bitpos_t retval = qrean_try_write_qr_data(qrean, buffer, len, data_type);
		if (retval > 0) return retval / 8;
	}
	return 0;
}

size_t qrean_read_qr_data(qrean_t *qrean, void *buffer, size_t size)
{
	qrpayload_t payload = {};
	qrpayload_init(&payload, qrean->qr.version, qrean->qr.level);
	qrean_read_qr_payload(qrean, &payload);

	size_t len = qrpayload_read_string(&payload, (char *)buffer, size, qrean->eci_code);

	qrpayload_deinit(&payload);
	return len;
}

size_t qrean_write_buffer(qrean_t *qrean, const char *buffer, size_t len, qrean_data_type_t data_type)
{
	if (qrean->code->write_data) {
		return qrean->code->write_data(qrean, buffer, len, data_type);
	}
	if (QREAN_IS_TYPE_QRFAMILY(qrean)) {
		return qrean_write_qr_data(qrean, buffer, len, data_type);
	}
	return 0;
}

size_t qrean_read_buffer(qrean_t *qrean, char *buffer, size_t size)
{
	if (qrean->code->read_data) {
		return qrean->code->read_data(qrean, buffer, size);
	}
	if (QREAN_IS_TYPE_QRFAMILY(qrean)) {
		return qrean_read_qr_data(qrean, buffer, size);
	}
	return 0;
}

int qrean_check_qr_combination(qrean_t *qrean)
{
	int min_v, max_v;
	if (qrean->qr.version == QR_VERSION_AUTO) {
		min_v = QREAN_IS_TYPE_QR(qrean) ? QR_VERSION_1 : QREAN_IS_TYPE_MQR(qrean) ? QR_VERSION_M1 : QR_VERSION_R7x43;
		max_v = QREAN_IS_TYPE_QR(qrean) ? QR_VERSION_40 : QREAN_IS_TYPE_MQR(qrean) ? QR_VERSION_M4 : QR_VERSION_R17x139;
	} else {
		min_v = max_v = qrean->qr.version;
	}

	for (int v = min_v; v <= max_v; v++) {
		if (qrspec_is_valid_combination((qr_version_t)v, qrean->qr.level, qrean->qr.mask)) return 1;
	}
	return 0;
}

const char *qrean_get_code_type_string(qrean_code_type_t code)
{
	switch (code) {
	case QREAN_CODE_TYPE_QR:
		return "QR";
	case QREAN_CODE_TYPE_MQR:
		return "mQR";
	case QREAN_CODE_TYPE_RMQR:
		return "rMQR";
	case QREAN_CODE_TYPE_TQR:
		return "tQR";
	case QREAN_CODE_TYPE_EAN13:
		return "EAN13";
	case QREAN_CODE_TYPE_EAN8:
		return "EAN8";
	case QREAN_CODE_TYPE_UPCA:
		return "UPCA";
	case QREAN_CODE_TYPE_CODE39:
		return "CODE39";
	case QREAN_CODE_TYPE_CODE93:
		return "CODE93";
	case QREAN_CODE_TYPE_NW7:
		return "NW7";
	case QREAN_CODE_TYPE_ITF:
		return "ITF";
	default:
		return "Unknown";
	}
}

qrean_code_type_t qrean_get_code_type_by_string(const char *str)
{
	if (!strcasecmp(str, "qr")) {
		return QREAN_CODE_TYPE_QR;
	} else if (!strcasecmp(str, "mqr")) {
		return QREAN_CODE_TYPE_MQR;
	} else if (!strcasecmp(str, "rmqr")) {
		return QREAN_CODE_TYPE_RMQR;
	} else if (!strcasecmp(str, "tqr")) {
		return QREAN_CODE_TYPE_TQR;
	} else if (!strcasecmp(str, "ean8") || !strcasecmp(str, "jan8") || !strcasecmp(str, "ean-8") || !strcasecmp(str, "jan-8")) {
		return QREAN_CODE_TYPE_EAN8;
	} else if (!strcasecmp(str, "ean13") || !strcasecmp(str, "jan13") || !strcasecmp(str, "ean-13") || !strcasecmp(str, "jan-13")) {
		return QREAN_CODE_TYPE_EAN13;
	} else if (!strcasecmp(str, "upca")) {
		return QREAN_CODE_TYPE_UPCA;
	} else if (!strcasecmp(str, "code39")) {
		return QREAN_CODE_TYPE_CODE39;
	} else if (!strcasecmp(str, "code93")) {
		return QREAN_CODE_TYPE_CODE93;
	} else if (!strcasecmp(str, "itf") || !strcasecmp(str, "i25")) {
		return QREAN_CODE_TYPE_ITF;
	} else if (!strcasecmp(str, "nw7") || !strcasecmp(str, "nw-7") || !strcasecmp(str, "codabar")) {
		return QREAN_CODE_TYPE_NW7;
	} else {
		return QREAN_CODE_TYPE_INVALID;
	}
}

bit_t qrean_set_eci_code(qrean_t *qrean, qr_eci_code_t code)
{
	qrean->eci_code = code;
	return 1;
}

qr_eci_code_t qrean_get_eci_code(qrean_t *qrean)
{
	return qrean->eci_code;
}

const char *qrean_version()
{
	return QREAN_VERSION; // defined in CFLAGS
}

bit_t qrean_set_qr_version_by_string(qrean_t *qrean, const char *version)
{
	qr_version_t v =  qrspec_get_version_by_string(version);
	if (v) return qrean_set_qr_version(qrean, v);
	return 0;
}

bit_t qrean_set_qr_errorlevel_by_string(qrean_t *qrean, const char *level)
{
	if (level) {
		switch (level[0]) {
		case 'l': case 'L': return qrean_set_qr_errorlevel(qrean, QR_ERRORLEVEL_L);
		case 'm': case 'M': return qrean_set_qr_errorlevel(qrean, QR_ERRORLEVEL_M);
		case 'h': case 'H': return qrean_set_qr_errorlevel(qrean, QR_ERRORLEVEL_H);
		case 'q': case 'Q': return qrean_set_qr_errorlevel(qrean, QR_ERRORLEVEL_Q);
		}
	}
	return 0;
}
