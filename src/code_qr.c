#include "bitstream.h"
#include "qrean.h"
#include "qrpayload.h"
#include "qrspec.h"
#include "qrtypes.h"
#include "runlength.h"

#define QR_FORMATINFO_SIZE        (15)
#define QR_VERSIONINFO_SIZE       (18)
#define QR_FINDER_PATTERN_SIZE    (9 * 9)
#define QR_ALIGNMENT_PATTERN_SIZE (5 * 5)

static bit_t is_mask(int_fast16_t x, int_fast16_t y, qr_maskpattern_t mask)
{
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

static bit_t is_format_info(qrean_t *qrean, int_fast16_t x, int_fast16_t y)
{
	if (y == 8 && ((0 <= x && x <= 8) || (qrean->canvas.symbol_width - 8 <= x && x < qrean->canvas.symbol_width))) return 1;
	if (x == 8 && ((0 <= y && y <= 8) || (qrean->canvas.symbol_height - 8 <= y && y < qrean->canvas.symbol_height))) return 1;

	return 0;
}

static bitpos_t format_info_iter(bitstream_t *bs, bitpos_t i, void *opaque)
{
	qrean_t *qrean = (qrean_t *)opaque;
	uint_fast8_t n = i / QR_FORMATINFO_SIZE;
	uint_fast8_t u = i % QR_FORMATINFO_SIZE;

	if (n >= 2) return BITPOS_END;

	int x = n != 0 ? 8 : u <= 5 ? u : u == 6 ? 7 : qrean->canvas.symbol_width + u - 15;
	int y = n == 0 ? 8 : u <= 6 ? qrean->canvas.symbol_height - 1 - u : u <= 8 ? 15 - u : 14 - u;

	return QREAN_XYV_TO_BITPOS(qrean, x, y, (0x5412 & (0x4000 >> u)));
}

static bit_t is_version_info(qrean_t *qrean, int_fast16_t x, int_fast16_t y)
{
	if (qrean->qr.version < QR_VERSION_7) return 0;
	if (x < 7 && qrean->canvas.symbol_height - 11 <= y && y <= qrean->canvas.symbol_height - 9) return 1;
	if (y < 7 && qrean->canvas.symbol_width - 11 <= x && x <= qrean->canvas.symbol_width - 9) return 1;
	return 0;
}

static bitpos_t version_info_iter(bitstream_t *bs, bitpos_t i, void *opaque)
{
	qrean_t *qrean = (qrean_t *)opaque;
	if (qrean->qr.version < QR_VERSION_7) return BITPOS_END;

	uint_fast8_t n = i / QR_VERSIONINFO_SIZE;
	uint_fast8_t u = i % QR_VERSIONINFO_SIZE;

	if (n >= 2) return BITPOS_END;

	int x = n == 0 ? 5 - u / 3 : qrean->canvas.symbol_width - 9 - u % 3;
	int y = n == 0 ? qrean->canvas.symbol_height - 9 - u % 3 : 5 - u / 3;

	return QREAN_XYV_TO_BITPOS(qrean, x, y, 0);
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

static bit_t is_alignment_pattern(qrean_t *qrean, int_fast16_t x, int_fast16_t y)
{
	int w = 2;

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

	return QREAN_XYV_TO_BITPOS(qrean, cx - 2 + i % 5, cy - 2 + i / 5 % 5, 0);
}

static bitpos_t composed_data_iter(bitstream_t *bs, bitpos_t i, void *opaque)
{
	qrean_t *qrean = (qrean_t *)opaque;
	bitpos_t w = qrean->canvas.symbol_width;
	bitpos_t h = qrean->canvas.symbol_height;
	int_fast16_t x = (w - 1) - (i % 2) - 2 * (i / (2 * h)) - ((i >= (w - 7) * h) ? 1 : 0);
	int_fast16_t y = (i % (4 * h) < 2 * h) ? (h - 1) - (i / 2 % (2 * h)) : -h + (i / 2 % (2 * h));

	if (x < 0 || y < 0) return BITPOS_END;

	// avoid function pattern
	if (is_finder_pattern(qrean, x, y)) return BITPOS_TRUNC;
	if (is_alignment_pattern(qrean, x, y)) return BITPOS_TRUNC;
	if (is_timing_pattern(qrean, x, y)) return BITPOS_TRUNC;
	if (is_format_info(qrean, x, y)) return BITPOS_TRUNC;
	if (is_version_info(qrean, x, y)) return BITPOS_TRUNC;

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

static const uint8_t alignment_pattern_bits[] = {
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

static const uint8_t timing_pattern_bits[] = { 0xAA };

static bit_t check_N3(runlength_t *rl, bit_t v)
{
	if (!v && runlength_match_ratio(rl, 6, 1, 1, 3, 1, 1, 0) && runlength_get_count(rl, 0) / 4 >= runlength_get_count(rl, 1)) return 1;
	if (v && runlength_match_ratio(rl, 6, 0, 1, 1, 3, 1, 1) && runlength_get_count(rl, 5) / 4 >= runlength_get_count(rl, 4)) return 1;

	return 0;
}

static unsigned int qr_score(qrean_t *qrean)
{
	// penalty
	const int N1 = 3;
	const int N2 = 3;
	const int N3 = 40;
	const int N4 = 10;
	size_t score = 0;

	int dark_modules = 0;
	for (bitpos_t y = 0; y < qrean->canvas.symbol_height; y++) {
		for (int dir = 0; dir < 2; dir++) {
			bit_t last_v = 2; // XXX:
			runlength_t rl = create_runlength();

			bit_t v;
			for (bitpos_t x = 0; x < qrean->canvas.symbol_width; x++) {
				v = qrean_read_pixel(qrean, dir ? y : x, dir ? x : y);

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
					qrean_read_pixel(qrean, x, y),
					qrean_read_pixel(qrean, x + 1, y),
					qrean_read_pixel(qrean, x, y + 1),
					qrean_read_pixel(qrean, x + 1, y + 1),
				};
				if (x + 1 < qrean->canvas.symbol_width && y + 1 < qrean->canvas.symbol_height && a[0] == a[1] && a[1] == a[2]
					&& a[2] == a[3]) {
					score += N2;
				}
			}

			if (runlength_latest_count(&rl) >= 5) score += runlength_latest_count(&rl) - 5 + N1;
			if (check_N3(&rl, last_v)) score += N3;
		}
	}

	int ratio = dark_modules * 100 / qrean->canvas.symbol_width / qrean->canvas.symbol_height;
	if (ratio < 50) {
		score += (50 - ratio) / 5 * N4;
	} else {
		score += (ratio - 50) / 5 * N4;
	}

	return score;
}

qrean_code_t qrean_code_qr = {
	.type = QREAN_CODE_TYPE_QR,

	.init = NULL,
	.score = qr_score,
	.data_iter = composed_data_iter,

	.qr = {
		 .format_info = {format_info_iter, NULL, QR_FORMATINFO_SIZE},
		 .version_info = {version_info_iter, NULL, QR_VERSIONINFO_SIZE},
		 .finder_pattern = {finder_pattern_iter, finder_pattern_bits, QR_FINDER_PATTERN_SIZE},
		 .timing_pattern = {timing_pattern_iter, timing_pattern_bits, 8},
		 .alignment_pattern = {alignment_pattern_iter, alignment_pattern_bits, QR_ALIGNMENT_PATTERN_SIZE},
	 },
};
