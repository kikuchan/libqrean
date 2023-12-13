#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitstream.h"
#include "debug.h"
#include "formatinfo.h"
#include "qrdata.h"
#include "qrmatrix.h"
#include "qrstream.h"
#include "versioninfo.h"

#define QR_XY_TO_BITPOS(qr, x, y) ((y) * (qr)->symbol_size + (x))
#define QR_XYV_TO_BITPOS(qr, x, y, v)                                             \
	(((x) < 0 || (y) < 0 || (x) >= (qr)->symbol_size || (y) >= (qr)->symbol_size) \
	     ? BITPOS_BLANK                                                           \
	     : ((QR_XY_TO_BITPOS((qr), (x), (y)) & BITPOS_MASK) | ((v) ? BITPOS_TOGGLE : 0)))

#define QR_BUFFER_SIZE(qr) (((qr)->symbol_size * (qr)->symbol_size + 7) / 8)

bit_t qrmatrix_init(qrmatrix_t *qr, qr_version_t version, qr_errorlevel_t level, qr_maskpattern_t mask) {
	assert(QR_VERSION_1 <= version && version <= QR_VERSION_40);

	if (version == QR_VERSION_AUTO) {
		debug_printf(BANNER "Cannot use auto mode without string\n");
		return 0;
	}
	if (version < QR_VERSION_1 || version > QR_VERSION_40) {
		debug_printf(BANNER "Invalid version of QR\n");
		return 0;
	}

	int symbol_size = SYMBOL_SIZE_FOR(version);

	qr->symbol_size = symbol_size;
	qr->version = version;
	qr->level = level;
	qr->mask = mask;

#ifndef NO_QRMATRIX_BUFFER
#if defined(USE_MALLOC_BUFFER) && !defined(NO_MALLOC)
	qr->buffer = (uint8_t *)malloc(QR_BUFFER_SIZE(qr));
#endif
	memset(qr->buffer, 0, QR_BUFFER_SIZE(qr));
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

qrmatrix_t create_qrmatrix(qr_version_t version, qr_errorlevel_t level, qr_maskpattern_t mask) {
	qrmatrix_t qr = {};
	qrmatrix_init(&qr, version, level, mask);
	return qr;
}

void qrmatrix_destroy(qrmatrix_t *qr) {
	qrmatrix_deinit(qr);
}

qrmatrix_t *new_qrmatrix(qr_version_t version, qr_errorlevel_t level, qr_maskpattern_t mask) {
#ifndef NO_MALLOC
	qrmatrix_t *qr = (qrmatrix_t *)malloc(sizeof(qrmatrix_t));
	if (!qr) return NULL;
	if (!qrmatrix_init(qr, version, level, mask)) {
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

void qrmatrix_put_bit_at(bitstream_t *bs, bitpos_t pos, void *opaque, bit_t v) {
	qrmatrix_t *qr = (qrmatrix_t *)opaque;
#ifndef NO_CALLBACK
	if (qr->put_pixel) {
		bitpos_t x = pos % qr->symbol_size;
		bitpos_t y = pos / qr->symbol_size;

		if (qr->put_pixel(qr, x, y, pos, v)) return;
	}
#endif

#ifndef NO_QRMATRIX_BUFFER
	if (v) {
		qr->buffer[pos / 8] |= (0x80 >> (pos % 8));
		;
	} else {
		qr->buffer[pos / 8] &= ~(0x80 >> (pos % 8));
		;
	}
#endif
}

bit_t qrmatrix_get_bit_at(bitstream_t *bs, bitpos_t pos, void *opaque) {
	qrmatrix_t *qr = (qrmatrix_t *)opaque;
#ifndef NO_CALLBACK
	if (qr->get_pixel) {
		bitpos_t x = pos % qr->symbol_size;
		bitpos_t y = pos / qr->symbol_size;
		return qr->get_pixel(qr, x, y, pos);
	} else
#endif
	{
#ifndef NO_QRMATRIX_BUFFER
		return qr->buffer[pos / 8] & (0x80 >> (pos % 8)) ? 1 : 0;
#else
		return 0;
#endif
	}
}

void qrmatrix_put_pixel(qrmatrix_t *qr, int_fast8_t x, int_fast8_t y, bit_t v) {
	if (x < 0) return;
	if (y < 0) return;
	if (x >= qr->symbol_size) return;
	if (y >= qr->symbol_size) return;

	bitpos_t pos = QR_XY_TO_BITPOS(qr, x, y);
	qrmatrix_put_bit_at(NULL, pos, qr, v);
}

bit_t qrmatrix_get_pixel(qrmatrix_t *qr, int_fast8_t x, int_fast8_t y) {
	if (x < 0) return 0;
	if (y < 0) return 0;
	if (x >= qr->symbol_size) return 0;
	if (y >= qr->symbol_size) return 0;

	bitpos_t pos = QR_XY_TO_BITPOS(qr, x, y);
	return qrmatrix_get_bit_at(NULL, pos, qr);
}

static bit_t is_mask(int_fast8_t x, int_fast8_t y, int mask) {
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

	uint_fast8_t n = i / VERSIONINFO_SIZE;
	if (n >= 2) return BITPOS_END;

	return QR_XYV_TO_BITPOS(qr, n == 0 ? i / 3 % 6 : qr->symbol_size - 11 + i % 3, n == 0 ? qr->symbol_size - 11 + i % 3 : i / 3 % 6, 0);
}

static bit_t is_version_info(qrmatrix_t *qr, int_fast8_t x, int_fast8_t y) {
	if (qr->version < 7) return 0;
	if (x < 7 && qr->symbol_size - 11 <= y && y <= qr->symbol_size - 9) return 1;
	if (y < 7 && qr->symbol_size - 11 <= x && x <= qr->symbol_size - 9) return 1;
	return 0;
}

static bitpos_t format_info_iter(bitstream_t *bs, bitpos_t i, void *opaque) {
	qrmatrix_t *qr = (qrmatrix_t *)opaque;
	uint_fast8_t n = i / FORMATINFO_SIZE;
	uint_fast8_t u = i % FORMATINFO_SIZE;

	if (n >= 2) return BITPOS_END;

	int x = n != 0 ? 8 : u <= 5 ? u : u == 6 ? 7 : qr->symbol_size + u - 15;
	int y = n == 0 ? 8 : u <= 6 ? qr->symbol_size - 1 - u : u <= 8 ? 15 - u : 14 - u;

	return QR_XYV_TO_BITPOS(qr, x, y, (0x5412 & (0x4000 >> u)));
}

static bit_t is_format_info(qrmatrix_t *qr, int_fast8_t x, int_fast8_t y) {
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

static bit_t is_timing_pattern(qrmatrix_t *qr, int_fast8_t x, int_fast8_t y) {
	if (y == 6 || x == 6) return 1;
	return 0;
}

#define FINDER_PATTERN_SIZE (9 * 9)
static bitpos_t finder_pattern_iter(bitstream_t *bs, bitpos_t i, void *opaque) {
	qrmatrix_t *qr = (qrmatrix_t *)opaque;
	int n = i / FINDER_PATTERN_SIZE;
	if (n >= 3) return BITPOS_END;

	int x = (i / 9 % 9) + (n == 1 ? (qr->symbol_size - 8) : -1);
	int y = (i % 9) + (n == 2 ? (qr->symbol_size - 8) : -1);

	return QR_XYV_TO_BITPOS(qr, x, y, 0);
}

static bit_t is_finder_pattern(qrmatrix_t *qr, int_fast8_t x, int_fast8_t y) {
	if (x < 8 && y < 8) return 1;
	if (x < 8 && y >= qr->symbol_size - 8) return 1;
	if (x >= qr->symbol_size - 8 && y < 8) return 1;
	return 0;
}

static uint_fast8_t alignment_addr(uint_fast8_t version, uint_fast8_t idx) {
	if (version <= 1) return 0;
	uint_fast8_t N = version / 7 + 2;
	uint_fast8_t r = ((((version + 1) * 8 / (N - 1)) + 3) / 4) * 2 * (N - idx - 1);
	uint_fast8_t v4 = version * 4;

	if (idx >= N) return 0;

	return v4 < r ? 6 : v4 - r + 10;
}

static bit_t is_alignment_pattern(qrmatrix_t *qr, int_fast8_t x, int_fast8_t y) {
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
	bitpos_t x = (n - 1) - (i % 2) - 2 * (i / (2 * n)) - ((i >= (n - 7) * n) ? 1 : 0);
	bitpos_t y = (i % (4 * n) < 2 * n) ? (n - 1) - (i / 2 % (2 * n)) : -n + (i / 2 % (2 * n));

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

void qrmatrix_dump(qrmatrix_t *qr, int padding) {
#ifndef NO_PRINTF
	const char *white = "\033[47m  \033[m";
	const char *black = "  ";
	for (int y = -padding; y < qr->symbol_size + padding; y++) {
		for (int x = -padding; x < qr->symbol_size + padding; x++) {
			printf("%s", qrmatrix_get_pixel(qr, x, y) ? black : white);
		}
		printf("\n");
	}
#endif
}

bitstream_t qrmatrix_create_bitstream(qrmatrix_t *qr, bitstream_iterator_t iter) {
	bitstream_t bs = create_bitstream(qr->buffer, qr->symbol_size * qr->symbol_size, iter, qr);
	bitstream_on_put_bit(&bs, qrmatrix_put_bit_at, qr);
	bitstream_on_get_bit(&bs, qrmatrix_get_bit_at, qr);
	return bs;
}

bitstream_t qrmatrix_create_bitstream_for_composed_data(qrmatrix_t *qr) {
	bitstream_t bs = qrmatrix_create_bitstream(qr, composed_data_iter);
	bitstream_on_put_bit(&bs, qrmatrix_put_bit_at, qr);
	bitstream_on_get_bit(&bs, qrmatrix_get_bit_at, qr);
	return bs;
}

// format info
void qrmatrix_put_format_info(qrmatrix_t *qr) {
	bitstream_t bs = qrmatrix_create_bitstream(qr, format_info_iter);
	formatinfo_t fi = create_formatinfo(qr->level, qr->mask);

	bitstream_put_bits(&bs, fi.value, FORMATINFO_SIZE);
	bitstream_put_bits(&bs, fi.value, FORMATINFO_SIZE);

	// XXX: dark module
	qrmatrix_put_pixel(qr, 8, qr->symbol_size - 8, 1);
}

void qrmatrix_put_version_info(qrmatrix_t *qr) {
	bitstream_t bs = qrmatrix_create_bitstream(qr, version_info_iter);
	versioninfo_t vi = create_versioninfo(qr->version);

	bitstream_put_bits(&bs, vi.value, VERSIONINFO_SIZE);
	bitstream_put_bits(&bs, vi.value, VERSIONINFO_SIZE);
}

void qrmatrix_put_timing_pattern(qrmatrix_t *qr) {
	const uint8_t timing_pattern_bits[] = {0x55};
	bitstream_t timing_pattern_bs = qrmatrix_create_bitstream(qr, timing_pattern_iter);
	bitstream_write(&timing_pattern_bs, timing_pattern_bits, 8, 0);
}

void qrmatrix_put_finder_pattern(qrmatrix_t *qr) {
	static const uint8_t finder_pattern_bits[] = {
		0b00000000, 0b00111111, 0b10010000, 0b01001011, 0b10100101, 0b11010010, 0b11101001, 0b00000100, 0b11111110, 0b00000000, 0b00000000,
	};
	bitstream_t finder_pattern_bs = qrmatrix_create_bitstream(qr, finder_pattern_iter);
	bitstream_write(&finder_pattern_bs, finder_pattern_bits, FINDER_PATTERN_SIZE, 0);
}

void qrmatrix_put_alignment_pattern(qrmatrix_t *qr) {
	static const uint8_t alignment_pattern_bits[] = {
		0b11111100,
		0b01101011,
		0b00011111,
		0b10000000,
	};
	bitstream_t alignment_pattern_bs = qrmatrix_create_bitstream(qr, alignment_pattern_iter);
	bitstream_write(&alignment_pattern_bs, alignment_pattern_bits, ALIGNMENT_PATTERN_SIZE, 0);
}

bitpos_t qrmatrix_put_data(qrmatrix_t *qr, qrstream_t *qrs) {
	bitstream_t bs = qrmatrix_create_bitstream_for_composed_data(qr);
	bitstream_t src = qrstream_get_bitstream(qrs);
	return bitstream_copy(&bs, &src, 0, 0);
}

bitpos_t qrmatrix_get_data(qrmatrix_t *qr, qrstream_t *qrs) {
	bitstream_t bs = qrmatrix_create_bitstream_for_composed_data(qr);
	bitstream_t dst = qrstream_get_bitstream(qrs);
	return bitstream_copy(&dst, &bs, 0, 0);
}

void qrmatrix_put_function_patterns(qrmatrix_t *qr) {
	qrmatrix_put_finder_pattern(qr);
	qrmatrix_put_alignment_pattern(qr);
	qrmatrix_put_timing_pattern(qr);
	qrmatrix_put_format_info(qr);
	qrmatrix_put_version_info(qr);
}

bitpos_t qrmatrix_put_all(qrmatrix_t *qr, qrstream_t *qrs) {
	qrmatrix_put_function_patterns(qr);
	return qrmatrix_put_data(qr, qrs);
}

bitpos_t qrmatrix_put_string(qrmatrix_t *qr, const char *src) {
	qrstream_t qrs = create_qrstream_for_string(qr->version, qr->level, src);
	bitpos_t ret = qrmatrix_put_all(qr, &qrs);
	qrstream_destroy(&qrs);

	return ret;
}

#ifndef NO_MALLOC
qrmatrix_t *new_qrmatrix_for_string(qr_version_t version, qr_errorlevel_t level, const char *src) {
	qrstream_t *qrs = new_qrstream_for_string(version, level, src);

	// TODO: auto select mask pattern
	qrmatrix_t *qrm = new_qrmatrix(qrs->version, qrs->level, QR_MASKPATTERN_0);
	qrmatrix_put_all(qrm, qrs);

	qrstream_free(qrs);

	return qrm;
}
#endif

qrmatrix_t create_qrmatrix_for_string(qr_version_t version, qr_errorlevel_t level, const char *src) {
	qrstream_t qrs = create_qrstream_for_string(version, level, src);

	// TODO: auto select mask pattern
	qrmatrix_t qrm = create_qrmatrix(qrs.version, qrs.level, QR_MASKPATTERN_0);
	qrmatrix_put_all(&qrm, &qrs);

	qrstream_destroy(&qrs);

	return qrm;
}

int qrmatrix_fix_errors(qrmatrix_t *qr) {
	qrstream_t qrs = {};
	qrstream_init(&qrs, qr->version, qr->level);
	qrmatrix_get_data(qr, &qrs);

	int n = qrstream_fix_errors(&qrs);
	if (n > 0) {
		qrmatrix_put_data(qr, &qrs);
	}

	qrstream_deinit(&qrs);
	return n;
}

#ifndef NO_CALLBACK
void qrmatrix_on_put_pixel(qrmatrix_t *qr, bit_t (*put_pixel)(qrmatrix_t *qr, bitpos_t x, bitpos_t y, bitpos_t pos, bit_t v)) {
	qr->put_pixel = put_pixel;
}

void qrmatrix_on_get_pixel(qrmatrix_t *qr, bit_t (*get_pixel)(qrmatrix_t *qr, bitpos_t x, bitpos_t y, bitpos_t pos)) {
	qr->get_pixel = get_pixel;
}
#endif

size_t qrmatrix_get_string(qrmatrix_t *qr, char *buffer, size_t size) {
	qrstream_t qrs = {};
	qrstream_init(&qrs, qr->version, qr->level);
	qrmatrix_get_data(qr, &qrs);

	qrdata_t qrdata = create_qrdata_for(&qrs);
	size_t len = qrdata_parse(&qrdata, buffer, size);
	if (len >= size) len = size - 1;
	buffer[len] = 0;

	qrstream_deinit(&qrs);

	return len;
}

size_t qrmatrix_get_bitmap(qrmatrix_t *qr, void *buffer, size_t size, bitpos_t bpp) {
	bitstream_t src = qrmatrix_create_bitstream(qr, NULL);
	bitstream_t dst = create_bitstream(buffer, size * 8, NULL, NULL);

	while (!bitstream_is_end(&src) && !bitstream_is_end(&dst)) {
		uint32_t v = bitstream_get_bit(&src) ? 0x00000000 : 0xffffffff;
		bitstream_put_bits(&dst, v, bpp);
	}
	return bitstream_tell(&dst) / 8;
}

size_t qrmatrix_put_bitmap(qrmatrix_t *qr, const void *buffer, size_t size, bitpos_t bpp) {
	bitstream_t dst = qrmatrix_create_bitstream(qr, NULL);
	bitstream_t src = create_bitstream(buffer, size * 8, NULL, NULL);

	while (!bitstream_is_end(&src) && !bitstream_is_end(&dst)) {
		bit_t v = bitstream_get_bits(&src, bpp) ? 0 : 1;
		bitstream_put_bit(&dst, v);
	}
	return bitstream_tell(&src) / 8;
}
