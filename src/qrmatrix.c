#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitstream.h"
#include "debug.h"
#include "qrdata.h"
#include "qrformat.h"
#include "qrmatrix.h"
#include "qrpayload.h"
#include "qrtypes.h"
#include "qrversion.h"
#include "runlength.h"

#define QR_XY_TO_BITPOS(qr, x, y) ((y)*QR_BUFFER_SIDE_LENGTH + (x))
#define QR_XYV_TO_BITPOS(qr, x, y, v)                                             \
	(((x) < 0 || (y) < 0 || (x) >= (qr)->symbol_size || (y) >= (qr)->symbol_size) \
	     ? BITPOS_BLANK                                                           \
	     : ((QR_XY_TO_BITPOS((qr), (x), (y)) & BITPOS_MASK) | ((v) ? BITPOS_TOGGLE : 0)))

bit_t qrmatrix_init(qrmatrix_t *qr) {
	qrmatrix_set_version(qr, QR_VERSION_AUTO);
	qrmatrix_set_errorlevel(qr, QR_ERRORLEVEL_M);
	qrmatrix_set_maskpattern(qr, QR_MASKPATTERN_AUTO);

	qrmatrix_set_padding(qr, create_padding1(2));

#ifndef NO_QRMATRIX_BUFFER
#if defined(USE_MALLOC_BUFFER) && !defined(NO_MALLOC)
	qr->buffer = (uint8_t *)malloc(QR_BUFFER_SIZE);
#endif
	memset(qr->buffer, 0, QR_BUFFER_SIZE);
#endif

	return 1;
}

void qrmatrix_deinit(qrmatrix_t *qr) {
#ifndef NO_QRMATRIX_BUFFER
#ifdef USE_MALLOC_BUFFER
	free(qr->buffer);
#endif
#endif
}

qrmatrix_t create_qrmatrix() {
	qrmatrix_t qr = {};
	qrmatrix_init(&qr);
	return qr;
}

void qrmatrix_destroy(qrmatrix_t *qr) {
	qrmatrix_deinit(qr);
}

qrmatrix_t *new_qrmatrix() {
#ifndef NO_MALLOC
	qrmatrix_t *qr = (qrmatrix_t *)malloc(sizeof(qrmatrix_t));
	if (!qr) return NULL;
	if (!qrmatrix_init(qr)) {
		free(qr);
		return NULL;
	}
	return qr;
#else
	return NULL;
#endif
}

void qrmatrix_free(qrmatrix_t *qr) {
#ifndef NO_MALLOC
	qrmatrix_deinit(qr);
	free(qr);
#endif
}

bit_t qrmatrix_set_version(qrmatrix_t *qr, qr_version_t version) {
	if (version == QR_VERSION_INVALID) {
		error("Invalid version");
		return 0;
	}
	qr->version = version;
	qr->symbol_size = SYMBOL_SIZE_FOR(version == QR_VERSION_AUTO ? MAX_QR_VERSION : version);
	qr->width = qr->symbol_size + qr->padding.l + qr->padding.r;
	qr->height = qr->symbol_size + qr->padding.t + qr->padding.b;
	return 1;
}

bit_t qrmatrix_set_maskpattern(qrmatrix_t *qr, qr_maskpattern_t mask) {
	if (mask == QR_MASKPATTERN_INVALID) return 0;
	qr->mask = mask;
	return 1;
}

bit_t qrmatrix_set_errorlevel(qrmatrix_t *qr, qr_errorlevel_t level) {
	if (level == QR_ERRORLEVEL_INVALID) return 0;
	qr->level = level;
	return 1;
}

bit_t qrmatrix_set_padding(qrmatrix_t *qr, padding_t padding) {
	// TODO:
	qr->padding = padding;
	qr->width = qr->padding.l + qr->symbol_size + qr->padding.r;
	qr->height = qr->padding.t + qr->symbol_size + qr->padding.b;
	return 1;
}

uint_fast8_t qrmatrix_get_width(qrmatrix_t *qr) {
	return qr->width;
}
uint_fast8_t qrmatrix_get_height(qrmatrix_t *qr) {
	return qr->width;
}

padding_t qrmatrix_get_padding(qrmatrix_t *qr) {
	return qr->padding;
}

void qrmatrix_fill(qrmatrix_t *qr, bit_t v) {
	bitstream_t bs = qrmatrix_create_bitstream(qr, NULL);
	bitstream_fill(&bs, v);
}

static void qrmatrix_write_bit_at(bitstream_t *bs, bitpos_t pos, void *opaque, bit_t v) {
	qrmatrix_t *qr = (qrmatrix_t *)opaque;
#ifndef NO_CALLBACK
	if (qr->write_pixel) {
		bitpos_t x = pos % QR_BUFFER_SIDE_LENGTH;
		bitpos_t y = pos / QR_BUFFER_SIDE_LENGTH;

		if (qr->write_pixel(qr, x, y, pos, v)) return;
	}
#endif

#ifndef NO_QRMATRIX_BUFFER
	WRITE_BIT(qr->buffer, pos, v);
#endif
}

static bit_t qrmatrix_read_bit_at(bitstream_t *bs, bitpos_t pos, void *opaque) {
	qrmatrix_t *qr = (qrmatrix_t *)opaque;
#ifndef NO_CALLBACK
	if (qr->read_pixel) {
		bitpos_t x = pos % QR_BUFFER_SIDE_LENGTH;
		bitpos_t y = pos / QR_BUFFER_SIDE_LENGTH;
		return qr->read_pixel(qr, x, y, pos);
	} else
#endif
	{
#ifndef NO_QRMATRIX_BUFFER
		return READ_BIT(qr->buffer, pos);
#else
		return 0;
#endif
	}
}

void qrmatrix_write_pixel(qrmatrix_t *qr, int x, int y, bit_t v) {
	if (x < 0) return;
	if (y < 0) return;
	if (x >= qr->symbol_size) return;
	if (y >= qr->symbol_size) return;

	bitpos_t pos = QR_XY_TO_BITPOS(qr, x, y);
	qrmatrix_write_bit_at(NULL, pos, qr, v);
}

bit_t qrmatrix_read_pixel(qrmatrix_t *qr, int x, int y) {
	if (x < 0) return 0;
	if (y < 0) return 0;
	if (x >= qr->symbol_size) return 0;
	if (y >= qr->symbol_size) return 0;

	bitpos_t pos = QR_XY_TO_BITPOS(qr, x, y);
	return qrmatrix_read_bit_at(NULL, pos, qr);
}

static bit_t is_mask(int_fast16_t x, int_fast16_t y, int mask) {
	switch (mask) {
	case QR_MASKPATTERN_0:
		return (y + x) % 2 == 0;
	case QR_MASKPATTERN_1:
		return y % 2 == 0;
	case QR_MASKPATTERN_2:
		return x % 3 == 0;
	case QR_MASKPATTERN_3:
		return (y + x) % 3 == 0;
	case QR_MASKPATTERN_4:
		return (y / 2 + x / 3) % 2 == 0;
	case QR_MASKPATTERN_5:
		return ((y * x) % 2) + ((y * x) % 3) == 0;
	case QR_MASKPATTERN_6:
		return (((y * x) % 2) + ((y * x) % 3)) % 2 == 0;
	case QR_MASKPATTERN_7:
		return (((y * x) % 3) + ((y + x) % 2)) % 2 == 0;

	default:
	case QR_MASKPATTERN_NONE:
		return 0;
	case QR_MASKPATTERN_ALL:
		return 1;
	}
}

// iterators
static bitpos_t version_info_iter(bitstream_t *bs, bitpos_t i, void *opaque) {
	qrmatrix_t *qr = (qrmatrix_t *)opaque;
	if (qr->version < 7) return BITPOS_END;

	uint_fast8_t n = i / QR_VERSIONINFO_SIZE;
	uint_fast8_t u = i % QR_VERSIONINFO_SIZE;

	if (n >= 2) return BITPOS_END;

	int x = n == 0 ? 5 - u / 3 : qr->symbol_size - 9 - u % 3;
	int y = n == 0 ? qr->symbol_size - 9 - u % 3 : 5 - u / 3;

	return QR_XYV_TO_BITPOS(qr, x, y, 0);
}

static bit_t is_version_info(qrmatrix_t *qr, int_fast16_t x, int_fast16_t y) {
	if (qr->version < 7) return 0;
	if (x < 7 && qr->symbol_size - 11 <= y && y <= qr->symbol_size - 9) return 1;
	if (y < 7 && qr->symbol_size - 11 <= x && x <= qr->symbol_size - 9) return 1;
	return 0;
}

static bitpos_t format_info_iter(bitstream_t *bs, bitpos_t i, void *opaque) {
	qrmatrix_t *qr = (qrmatrix_t *)opaque;
	uint_fast8_t n = i / QR_FORMATINFO_SIZE;
	uint_fast8_t u = i % QR_FORMATINFO_SIZE;

	if (n >= 2) return BITPOS_END;

	int x = n != 0 ? 8 : u <= 5 ? u : u == 6 ? 7 : qr->symbol_size + u - 15;
	int y = n == 0 ? 8 : u <= 6 ? qr->symbol_size - 1 - u : u <= 8 ? 15 - u : 14 - u;

	return QR_XYV_TO_BITPOS(qr, x, y, (0x5412 & (0x4000 >> u)));
}

static bit_t is_format_info(qrmatrix_t *qr, int_fast16_t x, int_fast16_t y) {
	if (y == 8 && ((0 <= x && x <= 8) || (qr->symbol_size - 8 <= x && x < qr->symbol_size))) return 1;
	if (x == 8 && ((0 <= y && y <= 8) || (qr->symbol_size - 8 <= y && y < qr->symbol_size))) return 1;
	return 0;
}

// XXX: timing bitstream must be even for to easy implementation
static bitpos_t timing_pattern_iter(bitstream_t *bs, bitpos_t i, void *opaque) {
	qrmatrix_t *qr = (qrmatrix_t *)opaque;
	int n = i / (SYMBOL_SIZE_FOR(qr->version) - 7 * 2 - 1) % (SYMBOL_SIZE_FOR(qr->version) - 7 * 2 - 1);
	int u = i % (SYMBOL_SIZE_FOR(qr->version) - 7 * 2 - 1);

	if (n >= 2) return BITPOS_END;
	if (u > qr->symbol_size - 8 * 2) return BITPOS_TRUNC;

	return QR_XYV_TO_BITPOS(qr, n == 0 ? 7 + u : 6, n != 0 ? 7 + u : 6, 0);
}

static bit_t is_timing_pattern(qrmatrix_t *qr, int_fast16_t x, int_fast16_t y) {
	if (y == 6 || x == 6) return 1;
	return 0;
}

#define FINDER_PATTERN_SIZE (9 * 9)
static bitpos_t finder_pattern_iter(bitstream_t *bs, bitpos_t i, void *opaque) {
	qrmatrix_t *qr = (qrmatrix_t *)opaque;
	int n = i / FINDER_PATTERN_SIZE;
	if (n >= 3) return BITPOS_END;

	int x = (i % 9) + (n == 1 ? (qr->symbol_size - 8) : -1);
	int y = (i / 9 % 9) + (n == 2 ? (qr->symbol_size - 8) : -1);

	return QR_XYV_TO_BITPOS(qr, x, y, 0);
}

static bit_t is_finder_pattern(qrmatrix_t *qr, int_fast16_t x, int_fast16_t y) {
	if (x < 8 && y < 8) return 1;
	if (x < 8 && y >= qr->symbol_size - 8) return 1;
	if (x >= qr->symbol_size - 8 && y < 8) return 1;
	return 0;
}

static uint_fast8_t alignment_num(uint_fast8_t version) {
	if (version <= 1) return 0;
	int N = version / 7 + 2;
	return N * N - 3;
}

static uint_fast8_t alignment_addr(uint_fast8_t version, uint_fast8_t idx) {
	if (version <= 1) return 0;
	uint_fast8_t N = version / 7 + 2;
	uint_fast8_t r = ((((version + 1) * 8 / (N - 1)) + 3) / 4) * 2 * (N - idx - 1);
	uint_fast8_t v4 = version * 4;

	if (idx >= N) return 0;

	return v4 < r ? 6 : v4 - r + 10;
}

static bit_t is_alignment_pattern(qrmatrix_t *qr, int_fast16_t x, int_fast16_t y) {
	uint_fast8_t N = qr->version > 1 ? qr->version / 7 + 2 : 0;

	uint_fast8_t xidx = 0, yidx = 0;
	for (uint_fast8_t i = 0; i < N; i++) {
		uint_fast8_t addr = alignment_addr(qr->version, i);
		if (addr - 2 <= x && x <= addr + 2) xidx = i + 1;
		if (addr - 2 <= y && y <= addr + 2) yidx = i + 1;
	}

	if (xidx == 1 && yidx == 1) return 0;
	if (xidx == N && yidx == 1) return 0;
	if (xidx == 1 && yidx == N) return 0;

	return xidx != 0 && yidx != 0 ? 1 : 0;
}

#define ALIGNMENT_PATTERN_SIZE (5 * 5)
static bitpos_t alignment_pattern_iter(bitstream_t *bs, bitpos_t i, void *opaque) {
	qrmatrix_t *qr = (qrmatrix_t *)opaque;

	uint_fast8_t N = qr->version > 1 ? qr->version / 7 + 2 - 1 : 0;
	uint_fast8_t n = i / (5 * 5);

	if (ALIGNMENT_PATTERN_SIZE * 7 * 7 <= i) return BITPOS_END;

	uint_fast8_t x = n % 7;
	uint_fast8_t y = n / 7;

	if (x == 0 && y == 0) return BITPOS_TRUNC;
	if (x == N && y == 0) return BITPOS_TRUNC;
	if (x == 0 && y == N) return BITPOS_TRUNC;

	uint_fast8_t cy = alignment_addr(qr->version, y);
	uint_fast8_t cx = alignment_addr(qr->version, x);
	if (cx == 0 || cy == 0) return BITPOS_TRUNC;

	return QR_XYV_TO_BITPOS(qr, cx - 2 + i % 5, cy - 2 + i / 5 % 5, 0);
}

static bitpos_t composed_data_iter(bitstream_t *bs, bitpos_t i, void *opaque) {
	qrmatrix_t *qr = (qrmatrix_t *)opaque;
	bitpos_t n = qr->symbol_size;
	int_fast16_t x = (n - 1) - (i % 2) - 2 * (i / (2 * n)) - ((i >= (n - 7) * n) ? 1 : 0);
	int_fast16_t y = (i % (4 * n) < 2 * n) ? (n - 1) - (i / 2 % (2 * n)) : -n + (i / 2 % (2 * n));

	if (x < 0 || y < 0) return BITPOS_END;

	// avoid function pattern
	if (is_finder_pattern(qr, x, y)) return BITPOS_TRUNC;
	if (is_alignment_pattern(qr, x, y)) return BITPOS_TRUNC;
	if (is_timing_pattern(qr, x, y)) return BITPOS_TRUNC;
	if (is_format_info(qr, x, y)) return BITPOS_TRUNC;
	if (is_version_info(qr, x, y)) return BITPOS_TRUNC;

	bit_t v = is_mask(x, y, qr->mask);

	return QR_XYV_TO_BITPOS(qr, x, y, v);
}

static bitpos_t bitmap_iter(bitstream_t *bs, bitpos_t i, void *opaque) {
	qrmatrix_t *qr = (qrmatrix_t *)opaque;

	if (i >= qr->width * qr->height) return BITPOS_END;

	int_fast32_t x = (int_fast32_t)(i % qr->width) - qr->padding.l;
	int_fast32_t y = (int_fast32_t)(i / qr->width) - qr->padding.t;

	return QR_XYV_TO_BITPOS(qr, x, y, 0);
}

void qrmatrix_dump(qrmatrix_t *qr) {
#ifndef NO_PRINTF
	const char *dots[4] = {"\u2588", "\u2580", "\u2584", " "};

	int_fast16_t sx = -qr->padding.l;
	int_fast16_t sy = -qr->padding.t;
	int_fast16_t ex = (int_fast16_t)qr->symbol_size + qr->padding.r;
	int_fast16_t ey = (int_fast16_t)qr->symbol_size + qr->padding.b;
	for (int_fast16_t y = sy; y < ey; y += 2) {
		for (int_fast16_t x = sx; x < ex; x++) {
			bit_t u = qrmatrix_read_pixel(qr, x, y + 0);
			bit_t l = y + 1 >= ey ? 1 : qrmatrix_read_pixel(qr, x, y + 1);

			printf("%s", dots[((u << 1) | l) & 3]);
		}
		printf("\n");
	}
#endif
}

bitstream_t qrmatrix_create_bitstream(qrmatrix_t *qr, bitstream_iterator_t iter) {
	bitstream_t bs = create_bitstream(qr->buffer, QR_BUFFER_SIDE_LENGTH * QR_BUFFER_SIDE_LENGTH, iter, qr);
	bitstream_on_write_bit(&bs, qrmatrix_write_bit_at, qr);
	bitstream_on_read_bit(&bs, qrmatrix_read_bit_at, qr);
	return bs;
}

bitstream_t qrmatrix_create_bitstream_for_composed_data(qrmatrix_t *qr) {
	assert(QR_VERSION_1 <= qr->version && qr->version <= QR_VERSION_40);
	assert(QR_MASKPATTERN_0 <= qr->mask && qr->mask <= QR_MASKPATTERN_7);

	bitstream_t bs = qrmatrix_create_bitstream(qr, composed_data_iter);
	bitstream_on_write_bit(&bs, qrmatrix_write_bit_at, qr);
	bitstream_on_read_bit(&bs, qrmatrix_read_bit_at, qr);
	return bs;
}

static uint_fast8_t calc_pattern_mismatch_error_rate(bitstream_t *bs, const uint8_t *pattern, bitpos_t size, bitpos_t n) {
	bitpos_t error = 0;
	bitpos_t total = 0;

	uint8_t buf[BYTE_SIZE(size)];

	for (bitpos_t i = 0; !bitstream_is_end(bs) && (!n || i < n); i++) {
		bitpos_t l = bitstream_read(bs, buf, size, 1);
		error += hamming_distance_mem(buf, pattern, l);
		total += l;
	}

	return total ? error * 100 / total : 0;
}

// format info
void qrmatrix_write_format_info(qrmatrix_t *qr) {
	bitstream_t bs = qrmatrix_create_bitstream(qr, format_info_iter);
	qrformat_t fi = qrformat_for(qr->level, qr->mask);

	bitstream_write_bits(&bs, fi.value, QR_FORMATINFO_SIZE);
	bitstream_write_bits(&bs, fi.value, QR_FORMATINFO_SIZE);

	// XXX: dark module
	qrmatrix_write_pixel(qr, 8, qr->symbol_size - 8, 1);
}

qrformat_t qrmatrix_read_format_info(qrmatrix_t *qr) {
	bitstream_t bs = qrmatrix_create_bitstream(qr, format_info_iter);

	qrformat_t fi1 = qrformat_from(bitstream_read_bits(&bs, QR_FORMATINFO_SIZE));
	qrformat_t fi2 = qrformat_from(bitstream_read_bits(&bs, QR_FORMATINFO_SIZE));

	if (fi1.mask != QR_MASKPATTERN_INVALID) return fi1;
	return fi2;
}

bit_t qrmatrix_set_format_info(qrmatrix_t *qr, qrformat_t fi) {
	if (fi.level == QR_ERRORLEVEL_INVALID || fi.mask == QR_MASKPATTERN_INVALID) return 0;

	qr->level = fi.level;
	qr->mask = fi.mask;
	return 1;
}

bit_t qrmatrix_set_version_info(qrmatrix_t *qr, qrversion_t vi) {
	return qrmatrix_set_version(qr, vi.version);
}

void qrmatrix_write_version_info(qrmatrix_t *qr) {
	bitstream_t bs = qrmatrix_create_bitstream(qr, version_info_iter);
	qrversion_t vi = qrversion_for(qr->version);

	bitstream_write_bits(&bs, vi.value, QR_VERSIONINFO_SIZE);
	bitstream_write_bits(&bs, vi.value, QR_VERSIONINFO_SIZE);
}

qrversion_t qrmatrix_read_version_info(qrmatrix_t *qr) {
	bitstream_t bs = qrmatrix_create_bitstream(qr, version_info_iter);

	if (qr->version < 7) {
		// XXX: dummy
		return qrversion_for(qr->version);
		;
	}

	qrversion_t vi1 = qrversion_from(bitstream_read_bits(&bs, QR_VERSIONINFO_SIZE));
	qrversion_t vi2 = qrversion_from(bitstream_read_bits(&bs, QR_VERSIONINFO_SIZE));

	if (vi1.version != QR_VERSION_INVALID) return vi1;
	return vi2;
}

qr_version_t qrmatrix_read_version(qrmatrix_t *qr) {
	qrversion_t v = qrmatrix_read_version_info(qr);
	return v.version;
}

const uint8_t timing_pattern_bits[] = {0x55};
void qrmatrix_write_timing_pattern(qrmatrix_t *qr) {
	bitstream_t timing_pattern_bs = qrmatrix_create_bitstream(qr, timing_pattern_iter);
	bitstream_write(&timing_pattern_bs, timing_pattern_bits, 8, 0);
}

uint_fast8_t qrmatrix_read_timing_pattern(qrmatrix_t *qr) {
	bitstream_t bs = qrmatrix_create_bitstream(qr, timing_pattern_iter);
	int r = calc_pattern_mismatch_error_rate(&bs, timing_pattern_bits, 8, 0);
	return r;
}

static const uint8_t finder_pattern_bits[] = {
	0b00000000, 0b00111111, 0b10010000, 0b01001011, 0b10100101, 0b11010010, 0b11101001, 0b00000100, 0b11111110, 0b00000000, 0b00000000,
};
void qrmatrix_write_finder_pattern(qrmatrix_t *qr) {
	bitstream_t bs = qrmatrix_create_bitstream(qr, finder_pattern_iter);
	bitstream_write(&bs, finder_pattern_bits, FINDER_PATTERN_SIZE, 0);
}

uint_fast8_t qrmatrix_read_finder_pattern(qrmatrix_t *qr) {
	bitstream_t bs = qrmatrix_create_bitstream(qr, finder_pattern_iter);
	return calc_pattern_mismatch_error_rate(&bs, finder_pattern_bits, FINDER_PATTERN_SIZE, 3);
}

static const uint8_t alignment_pattern_bits[] = {
	0b11111100,
	0b01101011,
	0b00011111,
	0b10000000,
};

void qrmatrix_write_alignment_pattern(qrmatrix_t *qr) {
	bitstream_t bs = qrmatrix_create_bitstream(qr, alignment_pattern_iter);
	bitstream_write(&bs, alignment_pattern_bits, ALIGNMENT_PATTERN_SIZE, 0);
}

uint_fast8_t qrmatrix_read_alignment_pattern(qrmatrix_t *qr) {
	bitstream_t bs = qrmatrix_create_bitstream(qr, alignment_pattern_iter);
	uint_fast8_t n = alignment_num(qr->version);
	if (n == 0) return 0;
	return calc_pattern_mismatch_error_rate(&bs, alignment_pattern_bits, ALIGNMENT_PATTERN_SIZE, n);
}

bitpos_t qrmatrix_write_payload(qrmatrix_t *qr, qrpayload_t *qrp) {
	bitstream_t bs = qrmatrix_create_bitstream_for_composed_data(qr);
	bitstream_t src = qrpayload_get_bitstream(qrp);
	return bitstream_copy(&bs, &src, 0, 0);
}

bitpos_t qrmatrix_read_payload(qrmatrix_t *qr, qrpayload_t *qrp) {
	bitstream_t bs = qrmatrix_create_bitstream_for_composed_data(qr);
	bitstream_t dst = qrpayload_get_bitstream(qrp);
	return bitstream_copy(&dst, &bs, 0, 0);
}

void qrmatrix_write_function_patterns(qrmatrix_t *qr) {
	qrmatrix_write_finder_pattern(qr);
	qrmatrix_write_alignment_pattern(qr);
	qrmatrix_write_timing_pattern(qr);
	qrmatrix_write_format_info(qr);
	qrmatrix_write_version_info(qr);
}

static bit_t check_N3(runlength_t *rl, bit_t v) {
	if (!v && runlength_match_ratio(rl, 6, 1, 1, 3, 1, 1, 0) && runlength_get_count(rl, 0) / 4 >= runlength_get_count(rl, 1)) return 1;
	if (v && runlength_match_ratio(rl, 6, 0, 1, 1, 3, 1, 1) && runlength_get_count(rl, 5) / 4 >= runlength_get_count(rl, 4)) return 1;

	return 0;
}

unsigned int qrmatrix_score(qrmatrix_t *qrm) {
	// penalty
	const int N1 = 3;
	const int N2 = 3;
	const int N3 = 40;
	const int N4 = 10;
	size_t score = 0;

	int dark_modules = 0;
	for (bitpos_t y = 0; y < qrm->symbol_size; y++) {
		for (int dir = 0; dir < 2; dir++) {
			bit_t last_v = 2; // XXX:
			runlength_t rl = create_runlength();

			bit_t v;
			for (bitpos_t x = 0; x < qrm->symbol_size; x++) {
				v = qrmatrix_read_pixel(qrm, dir ? y : x, dir ? x : y);

				if (last_v != v) {
					if (runlength_latest_count(&rl) >= 5) score += runlength_latest_count(&rl) - 5 + N1;
					if (check_N3(&rl, last_v)) score += N3;

					runlength_next(&rl);
					last_v = v;
				}
				runlength_count(&rl);

				if (dir) continue;

				if (v) dark_modules++;

				int a[4] = {
					qrmatrix_read_pixel(qrm, x, y),
					qrmatrix_read_pixel(qrm, x + 1, y),
					qrmatrix_read_pixel(qrm, x, y + 1),
					qrmatrix_read_pixel(qrm, x + 1, y + 1),
				};
				if (x + 1 < qrm->symbol_size && y + 1 < qrm->symbol_size && a[0] == a[1] && a[1] == a[2] && a[2] == a[3]) {
					score += N2;
				}
			}

			if (runlength_latest_count(&rl) >= 5) score += runlength_latest_count(&rl) - 5 + N1;
			if (check_N3(&rl, last_v)) score += N3;
		}
	}

	int ratio = dark_modules * 100 / qrm->symbol_size / qrm->symbol_size;
	if (ratio < 50) {
		score += (50 - ratio) / 5 * N4;
	} else {
		score += (ratio - 50) / 5 * N4;
	}

	return score;
}

static qr_maskpattern_t select_maskpattern(qrmatrix_t *qrm, qrpayload_t *qrp) {
	unsigned int min_score = UINT_MAX;
	qr_maskpattern_t mask = qrm->mask;

	for (uint_fast8_t m = QR_MASKPATTERN_0; m <= QR_MASKPATTERN_7; m++) {
		qrmatrix_set_maskpattern(qrm, (qr_maskpattern_t)m);
		qrmatrix_write_all(qrm, qrp);
		unsigned int score = qrmatrix_score(qrm);
		if (min_score > score) {
			min_score = score;
			mask = (qr_maskpattern_t)m;
		}
	}

	return mask;
}

bitpos_t qrmatrix_write_all(qrmatrix_t *qrm, qrpayload_t *qrp) {
	if (qrm->mask == QR_MASKPATTERN_AUTO) {
		qrmatrix_set_maskpattern(qrm, select_maskpattern(qrm, qrp));
	}

	if (!qrmatrix_set_version(qrm, qrp->version)) {
		error("Invalid version");
		return 0;
	}
	qrmatrix_write_function_patterns(qrm);
	return qrmatrix_write_payload(qrm, qrp);
}

static bitpos_t qrmatrix_try_write_string_with_writer(qrmatrix_t *qrm, qr_version_t version, const char *src, size_t len,
                                                      qrdata_writer_t writer) {
	qrpayload_t qrp = {};
	qrpayload_init(&qrp, version, qrm->level);

	qrdata_t data = create_qrdata_for(qrpayload_get_bitstream_for_data(&qrp), version);
	if (writer(&data, src, len) == len && qrdata_finalize(&data)) {
		qrpayload_set_error_words(&qrp);

		qrmatrix_set_version(qrm, version);
		bitpos_t ret = qrmatrix_write_all(qrm, &qrp);
		qrpayload_deinit(&qrp);
		return ret;
	}
	return 0;
}

static size_t qrmatrix_write_buffer_with_writer(qrmatrix_t *qrm, const char *src, size_t len, qrdata_writer_t writer) {
	if (qrm->version == QR_VERSION_AUTO) {
		for (int i = QR_VERSION_1; i < QR_VERSION_40; i++) {
			bitpos_t ret = qrmatrix_try_write_string_with_writer(qrm, (qr_version_t)i, src, len, writer);
			if (ret > 0) return ret / 8;
		}
		return 0;
	}
	return qrmatrix_try_write_string_with_writer(qrm, qrm->version, src, len, writer) / 8;
}

size_t qrmatrix_write_string_numeric(qrmatrix_t *qrm, const char *src) {
	return qrmatrix_write_buffer_with_writer(qrm, src, strlen(src), qrdata_write_numeric_string);
}
size_t qrmatrix_write_string_alnum(qrmatrix_t *qrm, const char *src) {
	return qrmatrix_write_buffer_with_writer(qrm, src, strlen(src), qrdata_write_alnum_string);
}
size_t qrmatrix_write_string_8bit(qrmatrix_t *qrm, const char *src) {
	return qrmatrix_write_buffer_with_writer(qrm, src, strlen(src), qrdata_write_8bit_string);
}
size_t qrmatrix_write_string(qrmatrix_t *qrm, const char *src) {
	return qrmatrix_write_buffer_with_writer(qrm, src, strlen(src), qrdata_write_string);
}
size_t qrmatrix_write_buffer_numeric(qrmatrix_t *qrm, const char *src, size_t len) {
	return qrmatrix_write_buffer_with_writer(qrm, src, len, qrdata_write_numeric_string);
}
size_t qrmatrix_write_buffer_alnum(qrmatrix_t *qrm, const char *src, size_t len) {
	return qrmatrix_write_buffer_with_writer(qrm, src, len, qrdata_write_alnum_string);
}
size_t qrmatrix_write_buffer_8bit(qrmatrix_t *qrm, const char *src, size_t len) {
	return qrmatrix_write_buffer_with_writer(qrm, src, len, qrdata_write_8bit_string);
}
size_t qrmatrix_write_buffer(qrmatrix_t *qrm, const char *src, size_t len) {
	return qrmatrix_write_buffer_with_writer(qrm, src, len, qrdata_write_string);
}

#ifndef NO_MALLOC
qrmatrix_t *new_qrmatrix_for_string(qr_version_t version, qr_errorlevel_t level, qr_maskpattern_t mask, const char *src) {
	qrmatrix_t *qrm = new_qrmatrix();
	qrmatrix_write_string(qrm, src);
	return qrm;
}
#endif

qrmatrix_t create_qrmatrix_for_string(qr_version_t version, qr_errorlevel_t level, qr_maskpattern_t mask, const char *src) {
	// TODO: auto select mask pattern
	qrmatrix_t qrm = create_qrmatrix();
	qrmatrix_write_string(&qrm, src);
	return qrm;
}

int qrmatrix_fix_errors(qrmatrix_t *qr) {
	qrpayload_t qrp = {};
	qrpayload_init(&qrp, qr->version, qr->level);
	qrmatrix_read_payload(qr, &qrp);

	int n = qrpayload_fix_errors(&qrp);
	if (n > 0) {
		qrmatrix_write_payload(qr, &qrp);
	}

	qrpayload_deinit(&qrp);
	return n;
}

#ifndef NO_CALLBACK
void qrmatrix_on_write_pixel(qrmatrix_t *qr, bit_t (*write_pixel)(qrmatrix_t *qr, bitpos_t x, bitpos_t y, bitpos_t pos, bit_t v)) {
	qr->write_pixel = write_pixel;
}

void qrmatrix_on_read_pixel(qrmatrix_t *qr, bit_t (*read_pixel)(qrmatrix_t *qr, bitpos_t x, bitpos_t y, bitpos_t pos)) {
	qr->read_pixel = read_pixel;
}
#endif

size_t qrmatrix_read_string(qrmatrix_t *qr, char *buffer, size_t size) {
	qrpayload_t qrp = {};
	qrpayload_init(&qrp, qr->version, qr->level);
	qrmatrix_read_payload(qr, &qrp);

	qrdata_t qrdata = create_qrdata_for(qrpayload_get_bitstream_for_data(&qrp), qr->version);
	size_t len = qrdata_parse(&qrdata, buffer, size);
	if (len >= size) len = size - 1;
	buffer[len] = 0;

	qrpayload_deinit(&qrp);

	return len;
}

size_t qrmatrix_read_bitmap(qrmatrix_t *qr, void *buffer, size_t size, bitpos_t bpp) {
	bitstream_t src = qrmatrix_create_bitstream(qr, bitmap_iter);
	bitstream_t dst = create_bitstream(buffer, size * 8, NULL, NULL);

	while (!bitstream_is_end(&src) && !bitstream_is_end(&dst)) {
		uint32_t v = bitstream_read_bit(&src) ? 0x00000000 : 0xffffffff;
		bitstream_write_bits(&dst, v, bpp);
	}
	return bitstream_tell(&dst) / 8;
}

size_t qrmatrix_write_bitmap(qrmatrix_t *qr, const void *buffer, size_t size, bitpos_t bpp) {
	bitstream_t dst = qrmatrix_create_bitstream(qr, bitmap_iter);
	bitstream_t src = create_bitstream(buffer, size * 8, NULL, NULL);

	while (!bitstream_is_end(&src) && !bitstream_is_end(&dst)) {
		bit_t v = bitstream_read_bits(&src, bpp) ? 0 : 1;
		bitstream_write_bit(&dst, v);
	}
	return bitstream_tell(&src) / 8;
}
