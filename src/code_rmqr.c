#include "bitstream.h"
#include "debug.h"
#include "qrean.h"
#include "qrpayload.h"
#include "qrspec.h"

#define QR_FORMATINFO_SIZE            (18)
#define QR_FINDER_PATTERN_SIZE        (9 * 9)
#define QR_FINDER_SUB_PATTERN_SIZE    (5 * 5)
#define QR_ALIGNMENT_PATTERN_SIZE     (3 * 3)
#define QR_CORNER_FINDER_PATTERN_SIZE (6)

static bit_t is_mask(int_fast16_t j, int_fast16_t i, qr_maskpattern_t mask)
{
	switch (mask) {
	case QR_MASKPATTERN_0:
		return (i / 2 + j / 3) % 2 == 0;

	case QR_MASKPATTERN_NONE:
		return 0;
	case QR_MASKPATTERN_ALL:
		return 1;

	default:
		qrean_error("Invalid mask pattern");
		return 0;
	}
}

static bit_t is_format_info(qrean_t *qrean, int_fast16_t x, int_fast16_t y)
{
	if (8 <= x && x <= 10 && y <= 5) return 1;
	if (x == 11 && 0 <= y && y <= 3) return 1;

	int w = qrean->canvas.symbol_width;
	int h = qrean->canvas.symbol_height;
	if (h - 6 <= y && y < h && w - 8 <= x && x < w - 8 + 3) return 1;
	if (h - 6 == y && w - 8 + 3 <= x && x < w - 8 + 6) return 1;

	return 0;
}

static bitpos_t format_info_iter(bitstream_t *bs, bitpos_t i, void *opaque)
{
	qrean_t *qrean = (qrean_t *)opaque;
	uint_fast8_t n = i / QR_FORMATINFO_SIZE;
	uint_fast8_t u = (QR_FORMATINFO_SIZE - 1 - (i % QR_FORMATINFO_SIZE));

	if (n == 0) {
		int x = 8 + u / 5;
		int y = 1 + u % 5;
		return QREAN_XYV_TO_BITPOS(qrean, x, y, (0x1FAB2 & (1 << u)));
	} else if (n == 1) {
		int x, y;
		if (u < 15) {
			x = qrean->canvas.symbol_width - 8 + u / 5;
			y = qrean->canvas.symbol_height - 6 + u % 5;
		} else {
			x = qrean->canvas.symbol_width - 8 + (u - 12);
			y = qrean->canvas.symbol_height - 6;
		}
		return QREAN_XYV_TO_BITPOS(qrean, x, y, (0x20A7B & (1 << u)));
	}

	return BITPOS_END;
}

static bit_t is_finder_pattern(qrean_t *qrean, int_fast16_t x, int_fast16_t y)
{
	if (x < 8 && y < 8) return 1;
	if (QREAN_IS_TYPE_QR(qrean)) {
		if (x < 8 && y >= qrean->canvas.symbol_height - 8) return 1;
		if (x >= qrean->canvas.symbol_width - 8 && y < 8) return 1;
	}
	return 0;
}

static bitpos_t finder_pattern_iter(bitstream_t *bs, bitpos_t i, void *opaque)
{
	qrean_t *qrean = (qrean_t *)opaque;
	int n = i / QR_FINDER_PATTERN_SIZE;
	if (n >= (QREAN_IS_TYPE_RMQR(qrean) || QREAN_IS_TYPE_MQR(qrean) ? 1 : 3)) return BITPOS_END;

	int x = (i % 9) + (n == 1 ? (qrean->canvas.symbol_width - 8) : -1);
	int y = (i / 9 % 9) + (n == 2 ? (qrean->canvas.symbol_height - 8) : -1);

	return QREAN_XYV_TO_BITPOS(qrean, x, y, 0);
}

static bit_t is_horizontal_timing_pattern(qrean_t *qrean, int_fast16_t x, int_fast16_t y)
{
	if (y == 0 || y == qrean->canvas.symbol_height - 1) return 1;
	return 0;
}

static bit_t is_vertical_timing_pattern(qrean_t *qrean, int_fast16_t x, int_fast16_t y)
{
	if (x == 0 || x == qrean->canvas.symbol_width - 1) return 1;

	// XXX:
	uint_fast8_t N = qrspec_get_alignment_num(qrean->qr.version);
	for (int n = 0; n < N; n++) {
		if (x == qrspec_get_alignment_position_x(qrean->qr.version, n)) return 1;
	}

	return 0;
}

static bitpos_t timing_pattern_iter(bitstream_t *bs, bitpos_t i, void *opaque)
{
	qrean_t *qrean = (qrean_t *)opaque;

	int num_aligns_x = qrspec_get_alignment_num(qrean->qr.version) / 2; // XXX:

	// top
	if (i < (size_t)qrean->canvas.symbol_width - 8 - 3)
		return QREAN_XYV_TO_BITPOS(qrean, i + 8, 0, is_vertical_timing_pattern(qrean, i + 8, 0) // timing pattern dark module
		);
	i -= qrean->canvas.symbol_width - 8 - 3;

	// bottom
	bitpos_t ox = qrean->canvas.symbol_height > 2 + 5 ? 2 : 8;
	if (i < (size_t)qrean->canvas.symbol_width - ox - 5)
		return QREAN_XYV_TO_BITPOS(qrean, i + ox, qrean->canvas.symbol_height - 1,
			is_vertical_timing_pattern(qrean, i + ox, qrean->canvas.symbol_height - 1) // timing pattern dark module
		);
	i -= qrean->canvas.symbol_width - ox - 5;

	// left
	if (qrean->canvas.symbol_height > 8 + 3) {
		if (i < (size_t)qrean->canvas.symbol_height - 8 - 3) return QREAN_XYV_TO_BITPOS(qrean, 0, i + 8, 0);
		i -= qrean->canvas.symbol_height - 8 - 3;
	}

	// right
	if (qrean->canvas.symbol_height > 2 + 5) {
		if (i < (size_t)qrean->canvas.symbol_height - 2 - 5) return QREAN_XYV_TO_BITPOS(qrean, qrean->canvas.symbol_width - 1, i + 2, 0);
		i -= qrean->canvas.symbol_height - 2 - 5;
	}

	// vertical timing pattern
	int n = i / (qrean->canvas.symbol_height - 1);
	int y = i % (qrean->canvas.symbol_height - 1);

	if (n >= num_aligns_x) return BITPOS_END;

	int x = qrspec_get_alignment_position_x(qrean->qr.version, n * 2); // XXX:
	return QREAN_XYV_TO_BITPOS(qrean, x, y, 0);
}

static bit_t is_alignment_pattern(qrean_t *qrean, int_fast16_t x, int_fast16_t y)
{
	int w = 1; // for rMQR

	uint_fast8_t N = qrspec_get_alignment_num(qrean->qr.version);
	for (uint_fast8_t n = 0; n < N; n++) {
		uint_fast8_t cx = qrspec_get_alignment_position_x(qrean->qr.version, n);
		uint_fast8_t cy = qrspec_get_alignment_position_y(qrean->qr.version, n);
		if (cx - w <= x && x <= cx + w && cy - w <= y && y <= cy + w) return 1;
	}
	return 0;
}

static bitpos_t alignment_pattern_iter(bitstream_t *bs, bitpos_t i, void *opaque)
{
	qrean_t *qrean = (qrean_t *)opaque;

	uint_fast8_t n = i / QR_ALIGNMENT_PATTERN_SIZE;
	if (qrspec_get_alignment_num(qrean->qr.version) <= n) return BITPOS_END;

	uint_fast8_t cx = qrspec_get_alignment_position_x(qrean->qr.version, n);
	uint_fast8_t cy = qrspec_get_alignment_position_y(qrean->qr.version, n);
	if (cx == 0 || cy == 0) return BITPOS_TRUNC;

	return QREAN_XYV_TO_BITPOS(qrean, cx - 1 + i % 3, cy - 1 + i / 3 % 3, 0);
}

static bit_t is_finder_sub_pattern(qrean_t *qrean, int_fast16_t x, int_fast16_t y)
{
	if (IS_RMQR(qrean->qr.version)) {
		if (x >= qrean->canvas.symbol_width - 5 && y >= qrean->canvas.symbol_height - 5) return 1;
	}
	return 0;
}

static bitpos_t finder_sub_pattern_iter(bitstream_t *bs, bitpos_t i, void *opaque)
{
	qrean_t *qrean = (qrean_t *)opaque;
	int n = i / QR_FINDER_SUB_PATTERN_SIZE;
	if (n >= IS_RMQR(qrean->qr.version) ? 1 : 0) return BITPOS_END;

	int x = qrean->canvas.symbol_width - 1 - (i % 5);
	int y = qrean->canvas.symbol_height - 1 - (i / 5);

	return QREAN_XYV_TO_BITPOS(qrean, x, y, 0);
}

static bit_t is_corner_finder_pattern(qrean_t *qrean, int_fast16_t x, int_fast16_t y)
{
	if (3 >= (qrean->canvas.symbol_height - y) + (x)) return 1;
	if (3 >= (qrean->canvas.symbol_width - x) + (y)) return 1;
	return 0;
}

static bitpos_t corner_finder_pattern_iter(bitstream_t *bs, bitpos_t i, void *opaque)
{
	qrean_t *qrean = (qrean_t *)opaque;
	int n = i / QR_CORNER_FINDER_PATTERN_SIZE;
	int u = i % QR_CORNER_FINDER_PATTERN_SIZE;
	int flip = 0;

	if (n >= 2) return BITPOS_END;

	int x, y;
	if (n == 0) {
		x = u < 3 ? 0 : u < 5 ? 1 : 2;
		y = qrean->canvas.symbol_height - 1 - (u < 3 ? u : u < 5 ? u - 3 : 0);
		if (y == 7 && x == 0) flip = 1; // finder pattern white module
	} else {
		x = qrean->canvas.symbol_width - 1 - (u < 3 ? u : u < 5 ? u - 3 : 0);
		y = u < 3 ? 0 : u < 5 ? 1 : 2;
	}

	return QREAN_XYV_TO_BITPOS(qrean, x, y, flip);
}

static bitpos_t composed_data_iter(bitstream_t *bs, bitpos_t i, void *opaque)
{
	qrean_t *qrean = (qrean_t *)opaque;
	bitpos_t w = qrean->canvas.symbol_width;
	bitpos_t h = qrean->canvas.symbol_height;
	int_fast16_t x = (w - 1) - (i % 2) - 2 * (i / (2 * h)) - 1;
	int_fast16_t y = (i % (4 * h) < 2 * h) ? (h - 1) - (i / 2 % (2 * h)) : -h + (i / 2 % (2 * h));

	if (x < 0 || y < 0) return BITPOS_END;

	// avoid function pattern
	if (is_finder_pattern(qrean, x, y)) return BITPOS_TRUNC;
	if (is_finder_sub_pattern(qrean, x, y)) return BITPOS_TRUNC;
	if (is_corner_finder_pattern(qrean, x, y)) return BITPOS_TRUNC;
	if (is_alignment_pattern(qrean, x, y)) return BITPOS_TRUNC;
	if (is_vertical_timing_pattern(qrean, x, y)) return BITPOS_TRUNC;
	if (is_horizontal_timing_pattern(qrean, x, y)) return BITPOS_TRUNC;
	if (is_format_info(qrean, x, y)) return BITPOS_TRUNC;

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

static const uint8_t finder_sub_pattern_bits[] = {
	/*
	 * 11111
	 * 10001
	 * 10101
	 * 10001
	 * 11111
	 */
	0b11111100,
	0b01101011,
	0b00011111,
	0b10000000,
};

static const uint8_t alignment_pattern_bits[] = {
	/*
	 * 111
	 * 101
	 * 111
	 */
	0b11110111,
	0b10000000,
};

static const uint8_t corner_finder_pattern_bits[] = {
	/*
	 * 111
	 * 10
	 * 1
	 */
	0b11110100,
};

static const uint8_t timing_pattern_bits[] = { 0xAA };

qrean_code_t qrean_code_rmqr = {
	.type = QREAN_CODE_TYPE_RMQR,

	.init = NULL,
	.data_iter = composed_data_iter,

	.padding = { { 2, 2, 2, 2 } },

	.qr = {
		 .finder_pattern = {finder_pattern_iter, finder_pattern_bits, QR_FINDER_PATTERN_SIZE},
		 .finder_sub_pattern = {finder_sub_pattern_iter, finder_sub_pattern_bits, QR_FINDER_SUB_PATTERN_SIZE},
		 .corner_finder_pattern = {corner_finder_pattern_iter, corner_finder_pattern_bits, QR_CORNER_FINDER_PATTERN_SIZE},
		 .timing_pattern = {timing_pattern_iter, timing_pattern_bits, 8},
		 .alignment_pattern = {alignment_pattern_iter, alignment_pattern_bits, QR_ALIGNMENT_PATTERN_SIZE},
		 .format_info = {format_info_iter, NULL, QR_FORMATINFO_SIZE},
	 },
};
