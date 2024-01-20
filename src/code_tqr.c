#include "bitstream.h"
#include "debug.h"
#include "qrean.h"
#include "qrpayload.h"
#include "qrspec.h"

#define QR_FORMATINFO_SIZE     (15)
#define QR_FINDER_PATTERN_SIZE (9 * 9)
#define QR_BORDER_PATTERN_SIZE ((23 - 1) * 4)

#define BORDER_SIZE         (2)
#define INNER_SYMBOL_WIDTH  (19)
#define INNER_SYMBOL_HEIGHT (19)

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

static bitpos_t border_pattern_iter(bitstream_t *bs, bitpos_t i, void *opaque)
{
	qrean_t *qrean = (qrean_t *)opaque;
	if (i >= QR_BORDER_PATTERN_SIZE) return BITPOS_END;

	int x = i < 22 ? i : i < 22 * 2 ? 23 - 1 : i < 22 * 3 ? 23 - 1 - (i - 22 * 2) : 0;
	int y = i < 22 ? 0 : i < 22 * 2 ? i - 22 * 1 : i < 22 * 3 ? 23 - 1 : 23 - 1 - (i - 22 * 3);

	int v = ((30 < i && i < 38 && i % 2) || (50 < i && i < 58 && i % 2)) ? 1 : 0;

	return QREAN_XYV_TO_BITPOS(qrean, x, y, v);
}

static bitpos_t finder_pattern_iter(bitstream_t *bs, bitpos_t i, void *opaque)
{
	qrean_t *qrean = (qrean_t *)opaque;
	int n = i / QR_FINDER_PATTERN_SIZE;
	if (n >= 3) return BITPOS_END;

	int x = (i % 9) + (n == 1 ? (INNER_SYMBOL_WIDTH - 8) : -1);
	int y = (i / 9 % 9) + (n == 2 ? (INNER_SYMBOL_HEIGHT - 8) : -1);

	x += BORDER_SIZE;
	y += BORDER_SIZE;
	return QREAN_XYV_TO_BITPOS(qrean, x, y, 0);
}

static bitpos_t timing_pattern_iter(bitstream_t *bs, bitpos_t i, void *opaque)
{
	qrean_t *qrean = (qrean_t *)opaque;
	int n = i / (INNER_SYMBOL_WIDTH - 7 * 2 - 1) % (INNER_SYMBOL_WIDTH - 7 * 2 - 1);
	int u = i % (INNER_SYMBOL_WIDTH - 7 * 2 - 1);

	if (n >= 2) return BITPOS_END;

	if (u > INNER_SYMBOL_WIDTH - 8 * 2) return BITPOS_TRUNC;

	int x = n == 0 ? 7 + u : 6;
	int y = n != 0 ? 7 + u : 6;

	x += BORDER_SIZE;
	y += BORDER_SIZE;
	return QREAN_XYV_TO_BITPOS(qrean, x, y, 1);
}

static const struct {
	uint8_t x;
	uint8_t y;
} data_xypos[] = {
	{16, 18}, // block #1
	{17, 17}, // block #1
	{16, 17}, // block #1
	{18, 16}, // block #1
	{17, 16}, // block #1
	{16, 16}, // block #1
	{18, 15}, // block #1
	{17, 15}, // block #1
	{16, 15}, // block #1
	{18, 14}, // block #1
	{17, 14}, // block #2
	{16, 14}, // block #2
	{18, 13}, // block #2
	{17, 13}, // block #2
	{16, 13}, // block #2
	{18, 12}, // block #2
	{17, 12}, // block #2
	{16, 12}, // block #2
	{18, 11}, // block #2
	{17, 11}, // block #2
	{16, 11}, // block #3
	{18, 10}, // block #3
	{17, 10}, // block #3
	{16, 10}, // block #3
	{18,  9}, // block #3
	{17,  9}, // block #3
	{16,  9}, // block #3
	{18,  8}, // block #3
	{17,  8}, // block #3
	{16,  8}, // block #3
	{15,  9}, // block #4
	{14,  9}, // block #4
	{13,  9}, // block #4
	{12,  9}, // block #4
	{11,  9}, // block #4
	{15,  8}, // block #4
	{14,  8}, // block #4
	{13,  8}, // block #4
	{12,  8}, // block #4
	{11,  8}, // block #4
	{15, 11}, // block #5
	{14, 11}, // block #5
	{13, 11}, // block #5
	{12, 11}, // block #5
	{11, 11}, // block #5
	{15, 10}, // block #5
	{14, 10}, // block #5
	{13, 10}, // block #5
	{12, 10}, // block #5
	{11, 10}, // block #5
	{14, 15}, // block #6
	{13, 15}, // block #6
	{15, 14}, // block #6
	{14, 14}, // block #6
	{13, 14}, // block #6
	{15, 13}, // block #6
	{14, 13}, // block #6
	{13, 13}, // block #6
	{15, 12}, // block #6
	{14, 12}, // block #6
	{11, 15}, // block #7
	{10, 15}, // block #7
	{12, 14}, // block #7
	{11, 14}, // block #7
	{10, 14}, // block #7
	{12, 13}, // block #7
	{11, 13}, // block #7
	{13, 12}, // block #7
	{12, 12}, // block #7
	{11, 12}, // block #7
	{15, 18}, // block #8
	{14, 18}, // block #8
	{13, 18}, // block #8
	{15, 17}, // block #8
	{14, 17}, // block #8
	{13, 17}, // block #8
	{15, 16}, // block #8
	{14, 16}, // block #8
	{13, 16}, // block #8
	{15, 15}, // block #8
	{12, 18}, // block #9
	{11, 18}, // block #9
	{10, 18}, // block #9
	{12, 17}, // block #9
	{11, 17}, // block #9
	{10, 17}, // block #9
	{12, 16}, // block #9
	{11, 16}, // block #9
	{10, 16}, // block #9
	{12, 15}, // block #9
	{ 9, 18}, // block #10
	{ 8, 18}, // block #10
	{ 9, 17}, // block #10
	{ 8, 17}, // block #10
	{ 9, 16}, // block #10
	{ 8, 16}, // block #10
	{ 9, 15}, // block #10
	{ 8, 15}, // block #10
	{ 9, 14}, // block #10
	{ 8, 14}, // block #10
	{10, 13}, // block #11
	{ 9, 13}, // block #11
	{ 8, 13}, // block #11
	{10, 12}, // block #11
	{ 9, 12}, // block #11
	{ 8, 12}, // block #11
	{10, 11}, // block #11
	{ 9, 11}, // block #11
	{ 8, 11}, // block #11
	{10, 10}, // block #11
	{ 9, 10}, // block #12
	{ 8, 10}, // block #12
	{10,  9}, // block #12
	{ 9,  9}, // block #12
	{ 8,  9}, // block #12
	{10,  8}, // block #12
	{ 9,  8}, // block #12
	{ 8,  8}, // block #12
	{ 7,  8}, // block #12
	{ 8,  7}, // block #12
	{10,  7}, // block #13
	{ 9,  7}, // block #13
	{10,  5}, // block #13
	{ 9,  5}, // block #13
	{ 8,  5}, // block #13
	{10,  4}, // block #13
	{ 9,  4}, // block #13
	{ 8,  4}, // block #13
	{10,  3}, // block #13
	{ 9,  3}, // block #13
	{ 8,  3}, // block #14
	{10,  2}, // block #14
	{ 9,  2}, // block #14
	{ 8,  2}, // block #14
	{10,  1}, // block #14
	{ 9,  1}, // block #14
	{ 8,  1}, // block #14
	{10,  0}, // block #14
	{ 9,  0}, // block #14
	{ 8,  0}, // block #14
	{ 7, 10}, // block #15
	{ 5, 10}, // block #15
	{ 4, 10}, // block #15
	{ 7,  9}, // block #15
	{ 5,  9}, // block #15
	{ 4,  9}, // block #15
	{ 3,  9}, // block #15
	{ 5,  8}, // block #15
	{ 4,  8}, // block #15
	{ 3,  8}, // block #15
	{ 3, 10}, // block #16
	{ 2, 10}, // block #16
	{ 1, 10}, // block #16
	{ 0, 10}, // block #16
	{ 2,  9}, // block #16
	{ 1,  9}, // block #16
	{ 0,  9}, // block #16
	{ 2,  8}, // block #16
	{ 1,  8}, // block #16
	{ 0,  8}, // block #16
};

static bitpos_t composed_data_iter(bitstream_t *bs, bitpos_t i, void *opaque)
{
	qrean_t *qrean = (qrean_t *)opaque;

	if (i >= sizeof(data_xypos) / sizeof(data_xypos[0])) return BITPOS_END;
	int x = data_xypos[i].x;
	int y = data_xypos[i].y;

	bit_t v = is_mask(x, y, qrean->qr.mask);

	x += BORDER_SIZE;
	y += BORDER_SIZE;

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
static const uint8_t border_pattern_bits[] = { 0xFF };

qrean_code_t qrean_code_tqr = {
	.type = QREAN_CODE_TYPE_TQR,

	.init = NULL,
	.data_iter = composed_data_iter,

	.padding = { { 2, 2, 2, 2 } },

	.qr = {
		 .finder_pattern = {finder_pattern_iter, finder_pattern_bits, QR_FINDER_PATTERN_SIZE},
		 .border_pattern = {border_pattern_iter, border_pattern_bits, 8},
		 .timing_pattern = {timing_pattern_iter, timing_pattern_bits, 8},
	},
};
