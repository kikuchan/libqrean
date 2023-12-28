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
		// 1
		return i % 2 == 0;
	case QR_MASKPATTERN_1:
		// 4
		return (i / 2 + j / 3) % 2 == 0;
	case QR_MASKPATTERN_2:
		// 6
		return (((i * j) % 2) + ((i * j) % 3)) % 2 == 0;
	case QR_MASKPATTERN_3:
		// 7
		return (((i + j) % 2) + ((i + j) % 3)) % 2 == 0;

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
	if (y == 8 && (0 <= x && x <= 8)) return 1;
	if (x == 8 && (0 <= y && y <= 8)) return 1;
	return 0;
}

static bitpos_t format_info_iter(bitstream_t *bs, bitpos_t i, void *opaque)
{
	qrean_t *qrean = (qrean_t *)opaque;
	uint_fast8_t n = i / QR_FORMATINFO_SIZE;
	uint_fast8_t u = i % QR_FORMATINFO_SIZE;

	if (n >= 1) return BITPOS_END;

	int x = u < 7 ? u + 1 : 8;
	int y = u < 7 ? 8 : 15 - u;

	return QREAN_XYV_TO_BITPOS(qrean, x, y, (0x4445 & (0x4000 >> u)));
}

static bit_t is_finder_pattern(qrean_t *qrean, int_fast16_t x, int_fast16_t y)
{
	if (x < 8 && y < 8) return 1;
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

static bit_t is_timing_pattern(qrean_t *qrean, int_fast16_t x, int_fast16_t y)
{
	if (x == 0 || y == 0) return 1;
	return 0;
}

static bitpos_t timing_pattern_iter(bitstream_t *bs, bitpos_t i, void *opaque)
{
	qrean_t *qrean = (qrean_t *)opaque;
	int n = i / (qrean->canvas.symbol_width - 8 + 1) % (qrean->canvas.symbol_width - 8 + 1);
	int u = i % (qrean->canvas.symbol_width - 8 + 1);

	if (n >= 2) return BITPOS_END;

	if (u > qrean->canvas.symbol_width - 8) return BITPOS_TRUNC;
	return QREAN_XYV_TO_BITPOS(qrean, n == 0 ? 7 + u : 0, n != 0 ? 7 + u : 0, 1);
}

static bitpos_t composed_data_iter(bitstream_t *bs, bitpos_t i, void *opaque)
{
	qrean_t *qrean = (qrean_t *)opaque;
	bitpos_t w = qrean->canvas.symbol_width;
	bitpos_t h = qrean->canvas.symbol_height;
	int_fast16_t x = (w - 1) - (i % 2) - 2 * (i / (2 * h));
	int_fast16_t y = (i % (4 * h) < 2 * h) ? (h - 1) - (i / 2 % (2 * h)) : -h + (i / 2 % (2 * h));

	if (x < 0 || y < 0) return BITPOS_END;

	// avoid function pattern
	if (is_finder_pattern(qrean, x, y)) return BITPOS_TRUNC;
	if (is_timing_pattern(qrean, x, y)) return BITPOS_TRUNC;
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

static const uint8_t timing_pattern_bits[] = { 0xAA };

qrean_code_t qrean_code_mqr = {
	.type = QREAN_CODE_TYPE_MQR,

	.init = NULL,
	.data_iter = composed_data_iter,

	.qr = {
		 .finder_pattern = {finder_pattern_iter, finder_pattern_bits, QR_FINDER_PATTERN_SIZE},
		 .timing_pattern = {timing_pattern_iter, timing_pattern_bits, 8},
		 .format_info = {format_info_iter, NULL, QR_FORMATINFO_SIZE},
	 },
};
