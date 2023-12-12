#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "debug.h"
#include "bitstream.h"
#include "formatinfo.h"
#include "versioninfo.h"
#include "qrstream.h"
#include "qrdata.h"
#include "qrmatrix.h"

#define QR_XY_TO_BITPOS(qr, x, y) ((y) * (qr)->symbol_size + (x))
#define QR_XYV_TO_BITPOS(qr, x, y, v) (((x) < 0 || (y) < 0 || (x) >= (qr)->symbol_size || (y) >= (qr)->symbol_size) ? bitpos_blank : ((QR_XY_TO_BITPOS((qr), (x), (y)) & 0x7FFF) | ((v) ? 0x8000 : 0x0000)))

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

#ifndef NO_MALLOC
qrmatrix_t *new_qrmatrix(qr_version_t version, qr_errorlevel_t level, qr_maskpattern_t mask) {
	qrmatrix_t *qr = (qrmatrix_t *)malloc(sizeof(qrmatrix_t));
	if (!qr) return NULL;
	if (!qrmatrix_init(qr, version, level, mask)) {
		free(qr);
		return NULL;
	}
	return qr;
}

void qrmatrix_free(qrmatrix_t *qr) {
	qrmatrix_deinit(qr);
	free(qr);
}
#endif

void qrmatrix_set_bit_at(bitstream_t *bs, bitpos_t pos, void *opaque, bit_t v) {
	qrmatrix_t *qr = (qrmatrix_t *)opaque;
#ifndef NO_CALLBACK
	if (qr->set_pixel) {
		bitpos_t x = pos % qr->symbol_size;
		bitpos_t y = pos / qr->symbol_size;

		if (qr->set_pixel(qr, x, y, pos, v)) return;
	}
#endif

#ifndef NO_QRMATRIX_BUFFER
	if (v) {
		qr->buffer[pos / 8] |=  (0x80 >> (pos % 8));;
	} else {
		qr->buffer[pos / 8] &= ~(0x80 >> (pos % 8));;
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
#endif
	}
}

void qrmatrix_set_pixel(qrmatrix_t *qr, int_fast8_t x, int_fast8_t y, bit_t v) {
	if (x < 0) return;
	if (y < 0) return;
	if (x >= qr->symbol_size) return;
	if (y >= qr->symbol_size) return;

	bitpos_t pos = QR_XY_TO_BITPOS(qr, x, y);
	qrmatrix_set_bit_at(NULL, pos, qr, v);
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
	case QR_MASKPATTERN_0: return (y + x) % 2 == 0;
	case QR_MASKPATTERN_1: return y % 2 == 0;
	case QR_MASKPATTERN_2: return x % 3 == 0;
	case QR_MASKPATTERN_3: return (y + x) % 3 == 0;
	case QR_MASKPATTERN_4: return (y / 2 + x / 3) % 2 == 0;
	case QR_MASKPATTERN_5: return ((y * x) % 2) + ((y * x) % 3) == 0;
	case QR_MASKPATTERN_6: return (((y * x) % 2) + ((y * x) % 3)) % 2 == 0;
	case QR_MASKPATTERN_7: return (((y * x) % 3) + ((y + x) % 2)) % 2 == 0;

	default:
	case QR_MASKPATTERN_NONE: return 0;
	case QR_MASKPATTERN_ALL : return 1;
	}
}

// iterators
static bitpos_t version_info_iter(bitstream_t *bs, bitpos_t i, void *opaque) {
	qrmatrix_t *qr = (qrmatrix_t *)opaque;
	if (qr->version < 7) return bitpos_end;

	uint_fast8_t n = i / VERSIONINFO_SIZE;
	if (n >= 2) return bitpos_end;

	return QR_XYV_TO_BITPOS(qr,
		n == 0 ? i / 3 % 6 : qr->symbol_size - 11 + i % 3,
		n == 0 ? qr->symbol_size - 11 + i % 3 : i / 3 % 6,
		0
	);
}

static bit_t is_version_info(qrmatrix_t *qr, int_fast8_t x, int_fast8_t y)
{
	if (qr->version < 7) return 0;
	if (x < 7 && qr->symbol_size - 11 <= y && y <= qr->symbol_size - 9) return 1;
	if (y < 7 && qr->symbol_size - 11 <= x && x <= qr->symbol_size - 9) return 1;
	return 0;
}

static bitpos_t format_info_iter(bitstream_t *bs, bitpos_t i, void *opaque) {
	qrmatrix_t *qr = (qrmatrix_t *)opaque;
	uint_fast8_t n = i / FORMATINFO_SIZE;
	uint_fast8_t u = i % FORMATINFO_SIZE;

	if (n >= 2) return bitpos_end;

	int x = n != 0 ? 8 : u <= 5 ? u : u == 6 ? 7 : qr->symbol_size + u - 15;
	int y = n == 0 ? 8 : u <= 6 ? qr->symbol_size - 1 - u : u <= 8 ? 15 - u : 14 - u;

	return QR_XYV_TO_BITPOS(qr, x, y, (0x5412 & (0x4000 >> u)));
}

static bit_t is_format_info(qrmatrix_t *qr, int_fast8_t x, int_fast8_t y)
{
	if (y == 8 && ((0 <= x && x <= 8) || (qr->symbol_size - 8 <= x && x < qr->symbol_size))) return 1;
	if (x == 8 && ((0 <= y && y <= 8) || (qr->symbol_size - 8 <= y && y < qr->symbol_size))) return 1;
	return 0;
}


// XXX: timing bitstream must be even for to easy implementation
static bitpos_t timing_pattern_iter(bitstream_t *bs, bitpos_t i, void *opaque) {
	qrmatrix_t *qr = (qrmatrix_t *)opaque;
	int n = i / (SYMBOL_SIZE_FOR(qr->version) - 7 * 2 - 1) % (SYMBOL_SIZE_FOR(qr->version) - 7 * 2 - 1);
	int u = i % (SYMBOL_SIZE_FOR(qr->version) - 7 * 2 - 1);

	if (n >= 2) return bitpos_end;
	if (u > qr->symbol_size - 8 * 2) return bitpos_trunc;

	return QR_XYV_TO_BITPOS(qr,
		n == 0 ? 7 + u : 6,
		n != 0 ? 7 + u : 6,
		0
	);
}

static bit_t is_timing_pattern(qrmatrix_t *qr, int_fast8_t x, int_fast8_t y)
{
	if (y == 6 || x == 6) return 1;
	return 0;
}


#define FINDER_PATTERN_SIZE (9*9)
static bitpos_t finder_pattern_iter(bitstream_t *bs, bitpos_t i, void *opaque) {
	qrmatrix_t *qr = (qrmatrix_t *)opaque;
	int n = i / FINDER_PATTERN_SIZE;
	if (n >= 3) return bitpos_end;

	int x = (i / 9 % 9) + (n == 1 ? (qr->symbol_size - 8) : -1);
	int y = (i     % 9) + (n == 2 ? (qr->symbol_size - 8) : -1);

	return QR_XYV_TO_BITPOS(qr, x, y, 0);
}

static bit_t is_finder_pattern(qrmatrix_t *qr, int_fast8_t x, int_fast8_t y) {
	if (x < 8 && y < 8) return 1;
	if (x < 8 && y >= qr->symbol_size - 8) return 1;
	if (x >= qr->symbol_size - 8 && y < 8) return 1;
	return 0;
}

static const uint8_t alignment_pattern_addr[MAX_QR_VERSION][8] = {
	// version 1
	{0,0,0,0,0,0,0,0},
	{2,6,18,0,0,0,0,0},
	{2,6,22,0,0,0,0,0},
	{2,6,26,0,0,0,0,0},
	
	{2,6,30,0,0,0,0,0},
	{2,6,34,0,0,0,0,0},
	{3,6,22,38,0,0,0,0},
	{3,6,24,42,0,0,0,0},
	{3,6,26,46,0,0,0,0},
	
	// version 10
	{3,6,28,50,0,0,0,0},
	{3,6,30,54,0,0,0,0},
	{3,6,32,58,0,0,0,0},
	{3,6,34,62,0,0,0,0},
	{4,6,26,46,66,0,0,0},
	
	{4,6,26,48,70,0,0,0},
	{4,6,26,50,74,0,0,0},
	{4,6,30,54,78,0,0,0},
	{4,6,30,56,82,0,0,0},
	{4,6,30,58,86,0,0,0},
	
	// version 20
	{4,6,34,62,90,0,0,0},
	{5,6,28,50,72,94,0,0},
	{5,6,26,50,74,98,0,0},
	{5,6,30,54,78,102,0,0},
	{5,6,28,54,80,106,0,0},
	
	{5,6,32,58,84,110,0,0},
	{5,6,30,58,86,114,0,0},
	{5,6,34,62,90,118,0,0},
	{6,6,26,50,74,98,122,0},
	{6,6,30,54,78,102,126,0},
	
	// version 30
	{6,6,26,52,78,104,130,0},
	{6,6,30,56,82,108,134,0},
	{6,6,34,60,86,112,138,0},
	{6,6,30,58,86,114,142,0},
	{6,6,34,62,90,118,146,0},
	
	{7,6,30,54,78,102,126,150},
	{7,6,24,50,76,102,128,154},
	{7,6,28,54,80,106,132,158},
	{7,6,32,58,84,110,136,162},
	{7,6,26,54,82,110,138,166},
	
	// version 40
	{7,6,30,58,86,114,142,170}
};

static bit_t is_alignment_pattern(qrmatrix_t *qr, int_fast8_t x, int_fast8_t y)
{
	const uint8_t *addr = alignment_pattern_addr[qr->version - 1];
	int N = addr[0];

	uint8_t xidx = 0, yidx = 0;
	for (int i = 1; i <= N; i++) {
		if (addr[i] - 2 <= x && x <= addr[i] + 2) xidx = i;
		if (addr[i] - 2 <= y && y <= addr[i] + 2) yidx = i;
	}

	if (xidx == 1 && yidx == 1) return 0;
	if (xidx == N && yidx == 1) return 0;
	if (xidx == 1 && yidx == N) return 0;

	return xidx != 0 && yidx != 0 ? 1 : 0;
}

#define ALIGNMENT_PATTERN_SIZE (5 * 5)
static bitpos_t alignment_pattern_iter(bitstream_t *bs, bitpos_t i, void *opaque) {
	qrmatrix_t *qr = (qrmatrix_t *)opaque;
	const uint8_t *addr = alignment_pattern_addr[qr->version - 1];

	int N = addr[0] - 1;
	int n = i / (5 * 5);

	if (ALIGNMENT_PATTERN_SIZE * 7 * 7 <= i) return bitpos_end;

	int x = n % 7;
	int y = n / 7;
	if (x == 0 && y == 0) return bitpos_trunc;
	if (x == N && y == 0) return bitpos_trunc;
	if (x == 0 && y == N) return bitpos_trunc;

	uint8_t cy = addr[1 + y];
	uint8_t cx = addr[1 + x];
	if (cx == 0 || cy == 0) return bitpos_trunc;

	return QR_XYV_TO_BITPOS(qr,
		cx - 2 + i     % 5,
		cy - 2 + i / 5 % 5,
		0
	);
}

static bitpos_t composed_data_iter(bitstream_t *bs, bitpos_t i, void *opaque) {
	qrmatrix_t *qr = (qrmatrix_t *)opaque;
	bitpos_t n = qr->symbol_size;
	bitpos_t x = (n - 1) - (i % 2) - 2 * (i / (2 * n)) - ((i >= (n - 7) * n) ? 1 : 0);
	bitpos_t y = (i % (4 * n) < 2 * n) ? (n - 1) - (i / 2 % (2 * n)) : -n + (i / 2 % (2 * n));

	if (x < 0 || y < 0) return bitpos_end;

	// avoid function pattern
	if (is_finder_pattern   (qr, x, y)) return bitpos_trunc;
	if (is_alignment_pattern(qr, x, y)) return bitpos_trunc;
	if (is_timing_pattern   (qr, x, y)) return bitpos_trunc;
	if (is_format_info      (qr, x, y)) return bitpos_trunc;
	if (is_version_info     (qr, x, y)) return bitpos_trunc;

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
	bitstream_t bs = create_bitstream(qr->buffer, QR_BUFFER_SIZE(qr) * 8, iter, qr);
	bitstream_on_set(&bs, qrmatrix_set_bit_at, qr);
	bitstream_on_get(&bs, qrmatrix_get_bit_at, qr);
	return bs;
}

bitstream_t qrmatrix_create_bitstream_for_composed_data(qrmatrix_t *qr) {
	bitstream_t bs = qrmatrix_create_bitstream(qr, composed_data_iter);
	bitstream_on_set(&bs, qrmatrix_set_bit_at, qr);
	bitstream_on_get(&bs, qrmatrix_get_bit_at, qr);
	return bs;
}

// format info
void qrmatrix_write_format_info(qrmatrix_t *qr)
{
	bitstream_t bs = qrmatrix_create_bitstream(qr, format_info_iter);
	formatinfo_t fi = create_formatinfo(qr->level, qr->mask);

	bitstream_set_bits(&bs, fi.value, FORMATINFO_SIZE);
	bitstream_set_bits(&bs, fi.value, FORMATINFO_SIZE);

	// XXX: dark module
	qrmatrix_set_pixel(qr, 8, qr->symbol_size - 8, 1);
}

void qrmatrix_write_version_info(qrmatrix_t *qr)
{
	bitstream_t bs = qrmatrix_create_bitstream(qr, version_info_iter);
	versioninfo_t vi = create_versioninfo(qr->version);

	bitstream_set_bits(&bs, vi.value, VERSIONINFO_SIZE);
	bitstream_set_bits(&bs, vi.value, VERSIONINFO_SIZE);
}

void qrmatrix_write_timing_pattern(qrmatrix_t *qr)
{
	const uint8_t timing_pattern_bits[] = { 0x55 };
	bitstream_t timing_pattern_bs = qrmatrix_create_bitstream(qr, timing_pattern_iter);
	bitstream_write(&timing_pattern_bs, timing_pattern_bits, 8, 0);
}

void qrmatrix_write_finder_pattern(qrmatrix_t *qr)
{
	static const uint8_t finder_pattern_bits[] = {
		0b00000000,
		0b00111111,
		0b10010000,
		0b01001011,
		0b10100101,
		0b11010010,
		0b11101001,
		0b00000100,
		0b11111110,
		0b00000000,
		0b00000000,
	};
	bitstream_t finder_pattern_bs = qrmatrix_create_bitstream(qr, finder_pattern_iter);
	bitstream_write(&finder_pattern_bs, finder_pattern_bits, FINDER_PATTERN_SIZE, 0);
}

void qrmatrix_write_alignment_pattern(qrmatrix_t *qr)
{
	static const uint8_t alignment_pattern_bits[] = {
		0b11111100,
		0b01101011,
		0b00011111,
		0b10000000,
	};
	bitstream_t alignment_pattern_bs = qrmatrix_create_bitstream(qr, alignment_pattern_iter);
	bitstream_write(&alignment_pattern_bs, alignment_pattern_bits, ALIGNMENT_PATTERN_SIZE, 0);
}

void qrmatrix_write_data(qrmatrix_t *qr, qrstream_t *qrs)
{
	bitstream_t bs = qrmatrix_create_bitstream_for_composed_data(qr);
	bitstream_t src = qrstream_get_bitstream(qrs);
	bitstream_copy(&bs, &src, 0, 0);
}

void qrmatrix_read_data(qrmatrix_t *qr, qrstream_t *qrs)
{
	bitstream_t bs = qrmatrix_create_bitstream_for_composed_data(qr);
	bitstream_t dst = qrstream_get_bitstream(qrs);
	bitstream_copy(&dst, &bs, 0, 0);
}

void qrmatrix_write_function_patterns(qrmatrix_t *qr)
{
	qrmatrix_write_finder_pattern(qr);
	qrmatrix_write_alignment_pattern(qr);
	qrmatrix_write_timing_pattern(qr);
	qrmatrix_write_format_info(qr);
	qrmatrix_write_version_info(qr);
}

void qrmatrix_write_all(qrmatrix_t *qr, qrstream_t *qrs)
{
	qrmatrix_write_function_patterns(qr);
	qrmatrix_write_data(qr, qrs);
}

void qrmatrix_write_string(qrmatrix_t *qr, const char *src)
{
	qrstream_t qrs = create_qrstream_for_string(qr->version, qr->level, src);
	qrmatrix_write_all(qr, &qrs);
	qrstream_destroy(&qrs);
}

#ifndef NO_MALLOC
qrmatrix_t *new_qrmatrix_for_string(qr_version_t version, qr_errorlevel_t level, const char *src)
{
	qrstream_t *qrs = new_qrstream_for_string(version, level, src);

	// TODO: auto select mask pattern
	qrmatrix_t *qrm = new_qrmatrix(qrs->version, qrs->level, QR_MASKPATTERN_0);
	qrmatrix_write_all(qrm, qrs);

	qrstream_free(qrs);

	return qrm;
}
#endif

qrmatrix_t create_qrmatrix_for_string(qr_version_t version, qr_errorlevel_t level, const char *src)
{
	qrstream_t qrs = create_qrstream_for_string(version, level, src);

	// TODO: auto select mask pattern
	qrmatrix_t qrm = create_qrmatrix(qrs.version, qrs.level, QR_MASKPATTERN_0);
	qrmatrix_write_all(&qrm, &qrs);

	qrstream_destroy(&qrs);

	return qrm;
}

int qrmatrix_fix_errors(qrmatrix_t *qr)
{
	qrstream_t qrs = {};
	qrstream_init(&qrs, qr->version, qr->level);
	qrmatrix_read_data(qr, &qrs);

	int n = qrstream_fix_errors(&qrs);
	if (n > 0) {
		qrmatrix_write_data(qr, &qrs);
	}

	qrstream_deinit(&qrs);
	return n;
}

#ifndef NO_CALLBACK
void qrmatrix_on_set_pixel(qrmatrix_t *qr, bit_t (*set_pixel)(qrmatrix_t *qr, bitpos_t x, bitpos_t y, bitpos_t pos, bit_t v))
{
	qr->set_pixel = set_pixel;
}

void qrmatrix_on_get_pixel(qrmatrix_t *qr, bit_t (*get_pixel)(qrmatrix_t *qr, bitpos_t x, bitpos_t y, bitpos_t pos))
{
	qr->get_pixel = get_pixel;
}
#endif
