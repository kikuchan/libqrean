#include "bitstream.h"
#include "debug.h"
#include "qrean.h"
#include "qrpayload.h"
#include "qrspec.h"
#include "qrtypes.h"

#define QR_FORMATINFO_SIZE     (15)
#define QR_FINDER_PATTERN_SIZE (9 * 9)

static bit_t is_mask(int_fast16_t j, int_fast16_t i, qr_maskpattern_t mask)
{
	switch (mask) {
	case QR_MASKPATTERN_0:
		return (i + j) % 2 == 0;
	case QR_MASKPATTERN_1:
		return i % 2 == 0;
	case QR_MASKPATTERN_2:
		return j % 3 == 0;
	case QR_MASKPATTERN_3:
		return (i + j) % 3 == 0;
	case QR_MASKPATTERN_4:
		return (i / 2 + j / 3) % 2 == 0;
	case QR_MASKPATTERN_5:
		return ((i * j) % 2) + ((i * j) % 3) == 0;
	case QR_MASKPATTERN_6:
		return (((i * j) % 2) + ((i * j) % 3)) % 2 == 0;
	case QR_MASKPATTERN_7:
		return (((i * j) % 3) + ((i + j) % 2)) % 2 == 0;

	default:
	case QR_MASKPATTERN_NONE:
		return 0;
	case QR_MASKPATTERN_ALL:
		return 1;
	}
}

static bit_t is_finder_pattern(qrean_t *qrean, int_fast16_t x, int_fast16_t y)
{
	if (x < 8 && y < 8) return 1;
	if (x < 8 && y >= qrean->canvas.symbol_height - 8) return 1;
	if (x >= qrean->canvas.symbol_width - 8 && y < 8) return 1;
	return 0;
}

static bitpos_t finder_pattern_iter(bitstream_t *bs, bitpos_t i, void *opaque)
{
	qrean_t *qrean = (qrean_t *)opaque;
	int n = i / QR_FINDER_PATTERN_SIZE;
	if (n >= 3) return BITPOS_END;

	int x = (i % 9) + (n == 1 ? (qrean->canvas.symbol_width - 8) : -1);
	int y = (i / 9 % 9) + (n == 2 ? (qrean->canvas.symbol_height - 8) : -1);

	return QREAN_XYV_TO_BITPOS(qrean, x, y, 0);
}

static bit_t is_timing_pattern(qrean_t *qrean, int_fast16_t x, int_fast16_t y)
{
	if (x == 6 || y == 6) return 1;
	return 0;
}

static bitpos_t timing_pattern_iter(bitstream_t *bs, bitpos_t i, void *opaque)
{
	qrean_t *qrean = (qrean_t *)opaque;
	int n = i / (qrean->canvas.symbol_width - 7 * 2 - 1) % (qrean->canvas.symbol_width - 7 * 2 - 1);
	int u = i % (qrean->canvas.symbol_width - 7 * 2 - 1);

	if (n >= 2) return BITPOS_END;

	if (u > qrean->canvas.symbol_width - 8 * 2) return BITPOS_TRUNC;
	return QREAN_XYV_TO_BITPOS(qrean, n == 0 ? 7 + u : 6, n != 0 ? 7 + u : 6, 1);
}

static const struct {
	uint8_t x;
	uint8_t y;
} data_xypos[] = {
	{16, 18},
    {17, 17},
    {16, 17},
    {18, 16},
    {17, 16},
    {16, 16},
    {18, 15},
    {17, 15},
    {16, 15},
    {18, 14},
    {17, 14},
	{16, 14},
    {18, 13},
    {17, 13},
    {16, 13},
    {18, 12},
    {17, 12},
    {16, 12},
    {18, 11},
    {17, 11},
    {16, 11},
    {18, 10},
	{17, 10},
    {16, 10},
    {18,  9},
    {17,  9},
    {16,  9},
    {18,  8},
    {17,  8},
    {16,  8},
    {15,  9},
    {14,  9},
    {13,  9},
    {12,  9},
	{11,  9},
    {15,  8},
    {14,  8},
    {13,  8},
    {12,  8},
    {11,  8},
    {15, 11},
    {14, 11},
    {13, 11},
    {12, 11},
    {11, 11},
	{15, 10},
    {14, 10},
    {13, 10},
    {12, 10},
    {11, 10},
    {14, 15},
    {13, 15},
    {15, 14},
    {14, 14},
    {13, 14},
    {15, 13},
	{14, 13},
    {13, 13},
    {15, 12},
    {14, 12},
    {11, 15},
    {10, 15},
    {12, 14},
    {11, 14},
    {10, 14},
    {12, 13},
    {11, 13},
	{13, 12},
    {12, 12},
    {11, 12},
    {15, 18},
    {14, 18},
    {13, 18},
    {15, 17},
    {14, 17},
    {13, 17},
    {15, 16},
    {14, 16},
	{13, 16},
    {15, 15},
    {12, 18},
    {11, 18},
    {10, 18},
    {12, 17},
    {11, 17},
    {10, 17},
    {12, 16},
    {11, 16},
    {10, 16},
	{12, 15},
    { 9, 18},
    { 8, 18},
    { 9, 17},
    { 8, 17},
    { 9, 16},
    { 8, 16},
    { 9, 15},
    { 8, 15},
    { 9, 14},
    { 8, 14},
    {10, 13},
	{ 9, 13},
    { 8, 13},
    {10, 12},
    { 9, 12},
    { 8, 12},
    {10, 11},
    { 9, 11},
    { 8, 11},
    {10, 10},
    { 9, 10},
    { 8, 10},
    {10,  9},
	{ 9,  9},
    { 8,  9},
    {10,  8},
    { 9,  8},
    { 8,  8},
    { 7,  8},
    {10,  7},
    { 9,  7},
    { 8,  7},
    {10,  5},
    { 9,  5},
    { 8,  5},
    {10,  4},
	{ 9,  4},
    { 8,  4},
    {10,  3},
    { 9,  3},
    { 8,  3},
    {10,  2},
    { 9,  2},
    { 8,  2},
    {10,  1},
    { 9,  1},
    { 8,  1},
    {10,  0},
    { 9,  0},
	{ 8,  0},
    { 7, 10},
    { 5, 10},
    { 4, 10},
    { 7,  9},
    { 5,  9},
    { 4,  9},
    { 3,  9},
    { 5,  8},
    { 4,  8},
    { 3,  8},
    { 3, 10},
    { 2, 10},
	{ 1, 10},
    { 0, 10},
    { 2,  9},
    { 1,  9},
    { 0,  9},
    { 2,  8},
    { 1,  8},
    { 0,  8}
};

static bitpos_t composed_data_iter(bitstream_t *bs, bitpos_t i, void *opaque)
{
	qrean_t *qrean = (qrean_t *)opaque;

	if (i >= sizeof(data_xypos) / sizeof(data_xypos[0])) return BITPOS_END;
	int x = data_xypos[i].x;
	int y = data_xypos[i].y;

	bit_t v = is_mask(x, y, qrean->qr.mask);
	return QREAN_XYV_TO_BITPOS(qrean, x, y, v);
}

static const uint8_t finder_pattern_bits[] = {
	/*
	 * 000000000
	 * 011111110
	 * 010000010
	 * 010111010
	 * 010111010
	 * 010111010
	 * 010000010
	 * 011111110
	 * 000000000
	 */
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

static const uint8_t timing_pattern_bits[] = { 0xAA };

qrean_code_t qrean_code_tqr = {
	.type = QREAN_CODE_TYPE_TQR,

	.init = NULL,
	.data_iter = composed_data_iter,

	.qr = {
		 .finder_pattern = {finder_pattern_iter, finder_pattern_bits, QR_FINDER_PATTERN_SIZE},
		 .timing_pattern = {timing_pattern_iter, timing_pattern_bits, 8},
	},
};
