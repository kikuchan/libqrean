#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "bitstream.h"
#include "debug.h"
#include "galois.h"
#include "qrdata.h"
#include "qrstream.h"
#include "reedsolomon.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define QRSTREAM_BUFFER_SIZE(qrs) (((qrs)->total_bits + 7) / 8)

static const uint8_t QR_ERROR_WORDS_IN_BLOCK[40][4] = {
  //  L,  M,  Q,  H
	{ 7, 10, 13, 17}, //  1
	{10, 16, 22, 28}, //  2
	{15, 26, 18, 22}, //  3
	{20, 18, 26, 16}, //  4
	{26, 24, 18, 22}, //  5
	{18, 16, 24, 28}, //  6
	{20, 18, 18, 26}, //  7
	{24, 22, 22, 26}, //  8
	{30, 22, 20, 24}, //  9
	{18, 26, 24, 28}, // 10
	{20, 30, 28, 24}, // 11
	{24, 22, 26, 28}, // 12
	{26, 22, 24, 22}, // 13
	{30, 24, 20, 24}, // 14
	{22, 24, 30, 24}, // 15
	{24, 28, 24, 30}, // 16
	{28, 28, 28, 28}, // 17
	{30, 26, 28, 28}, // 18
	{28, 26, 26, 26}, // 19
	{28, 26, 30, 28}, // 20
	{28, 26, 28, 30}, // 21
	{28, 28, 30, 24}, // 22
	{30, 28, 30, 30}, // 23
	{30, 28, 30, 30}, // 24
	{26, 28, 30, 30}, // 25
	{28, 28, 28, 30}, // 26
	{30, 28, 30, 30}, // 27
	{30, 28, 30, 30}, // 28
	{30, 28, 30, 30}, // 29
	{30, 28, 30, 30}, // 30
	{30, 28, 30, 30}, // 31
	{30, 28, 30, 30}, // 32
	{30, 28, 30, 30}, // 33
	{30, 28, 30, 30}, // 34
	{30, 28, 30, 30}, // 35
	{30, 28, 30, 30}, // 36
	{30, 28, 30, 30}, // 37
	{30, 28, 30, 30}, // 38
	{30, 28, 30, 30}, // 39
	{30, 28, 30, 30}  // 40
};
;

static const uint8_t QR_TOTAL_RS_BLOCKS[40][4] = {
  //  L,  M,  Q,  H
	{ 1,  1,  1,  1}, //  1
	{ 1,  1,  1,  1}, //  2
	{ 1,  1,  2,  2}, //  3
	{ 1,  2,  2,  4}, //  4
	{ 1,  2,  4,  4}, //  5
	{ 2,  4,  4,  4}, //  6
	{ 2,  4,  6,  5}, //  7
	{ 2,  4,  6,  6}, //  8
	{ 2,  5,  8,  8}, //  9
	{ 4,  5,  8,  8}, // 10
	{ 4,  5,  8, 11}, // 11
	{ 4,  8, 10, 11}, // 12
	{ 4,  9, 12, 16}, // 13
	{ 4,  9, 16, 16}, // 14
	{ 6, 10, 12, 18}, // 15
	{ 6, 10, 17, 16}, // 16
	{ 6, 11, 16, 19}, // 17
	{ 6, 13, 18, 21}, // 18
	{ 7, 14, 21, 25}, // 19
	{ 8, 16, 20, 25}, // 20
	{ 8, 17, 23, 25}, // 21
	{ 9, 17, 23, 34}, // 22
	{ 9, 18, 25, 30}, // 23
	{10, 20, 27, 32}, // 24
	{12, 21, 29, 35}, // 25
	{12, 23, 34, 37}, // 26
	{12, 25, 34, 40}, // 27
	{13, 26, 35, 42}, // 28
	{14, 28, 38, 45}, // 29
	{15, 29, 40, 48}, // 30
	{16, 31, 43, 51}, // 31
	{17, 33, 45, 54}, // 32
	{18, 35, 48, 57}, // 33
	{19, 37, 51, 60}, // 34
	{19, 38, 53, 63}, // 35
	{20, 40, 56, 66}, // 36
	{21, 43, 59, 70}, // 37
	{22, 45, 62, 74}, // 38
	{24, 47, 65, 77}, // 39
	{25, 49, 68, 81}, // 40
};

static bitpos_t qrstream_available_bits(uint_fast8_t version) {
	bitpos_t symbol_size = 17 + 4 * version;

	bitpos_t finder_pattern = 8 * 8 * 3;
	bitpos_t N = version > 1 ? (version / 7) + 2 : 0; // alignment_pattern_addr[version - 1][0];
	bitpos_t alignment_pattern = version > 1 ? 5 * 5 * (N * N - 3) : 0;
	bitpos_t timing_pattern = (symbol_size - 8 * 2 - (version > 1 ? 5 * (N - 2) : 0)) * 2;
	bitpos_t version_info = version >= 7 ? 6 * 3 * 2 : 0;
	bitpos_t format_info = 15 * 2 + 1;

	bitpos_t function_bits = finder_pattern + alignment_pattern + timing_pattern + version_info + format_info;

	return symbol_size * symbol_size - function_bits;
}

void qrstream_init(qrstream_t *qrs, qr_version_t version, qr_errorlevel_t level) {
	assert(QR_VERSION_1 <= version && version <= QR_VERSION_40);
	assert(QR_ERRORLEVEL_L <= level && level <= QR_ERRORLEVEL_H);

	qrs->version = version;
	qrs->level = level;

	// counts
	qrs->total_bits = qrstream_available_bits(version);
	qrs->total_words = qrs->total_bits / 8;

	// refer to the known values
	qrs->total_blocks = QR_TOTAL_RS_BLOCKS[version - QR_VERSION_1][level];
	qrs->error_words_in_block = QR_ERROR_WORDS_IN_BLOCK[version - QR_VERSION_1][level];

	// let's do the simple math
	qrs->error_words = qrs->error_words_in_block * qrs->total_blocks;
	qrs->data_words = qrs->total_words - qrs->error_words;

	qrs->large_blocks = qrs->data_words % qrs->total_blocks;
	qrs->small_blocks = qrs->total_blocks - qrs->large_blocks;

	qrs->data_words_in_small_block = qrs->data_words / qrs->total_blocks;
	qrs->data_words_in_large_block = qrs->data_words_in_small_block + 1;

	qrs->error_words_in_block = qrs->error_words / qrs->total_blocks;

	qrs->total_words_in_small_block = qrs->data_words_in_small_block + qrs->error_words_in_block;
	qrs->total_words_in_large_block = qrs->data_words_in_large_block + qrs->error_words_in_block;

#if defined(USE_MALLOC_BUFFER) && !defined(NO_MALLOC)
	qrs->buffer = (uint8_t *)malloc(QRSTREAM_BUFFER_SIZE(qrs));
#endif
	memset(qrs->buffer, 0, QRSTREAM_BUFFER_SIZE(qrs));
}

void qrstream_deinit(qrstream_t *qrs) {
#if defined(USE_MALLOC_BUFFER) && !defined(NO_MALLOC)
	free(qrs->buffer);
#endif
}

qrstream_t create_qrstream(qr_version_t version, qr_errorlevel_t level) {
	qrstream_t qrs;
	qrstream_init(&qrs, version, level);
	return qrs;
}

void qrstream_destroy(qrstream_t *qrs) {
	qrstream_deinit(qrs);
}

qrstream_t *new_qrstream(qr_version_t version, qr_errorlevel_t level) {
#ifndef NO_MALLOC
	qrstream_t *qrs = (qrstream_t *)malloc(sizeof(qrstream_t));
	qrstream_init(qrs, version, level);
	return qrs;
#else
	return NULL;
#endif
}

void qrstream_free(qrstream_t *qrs) {
#ifndef NO_MALLOC
	free(qrs);
#endif
}

static int qrstream_try_for_string(qrstream_t *qrs, const char *src) {
	qrdata_t data = create_qrdata_for(qrs);
	size_t len = strlen(src);
	if (qrdata_write_string(&data, src, len) < len) return 0;
	if (qrdata_finalize(&data)) {
		return 1;
	}
	return 0;
}

qrstream_t *new_qrstream_for_string(qr_version_t version, qr_errorlevel_t level, const char *src) {
#ifndef NO_MALLOC
	if (version == QR_VERSION_AUTO) {
		for (int i = QR_VERSION_1; i <= QR_VERSION_40; i++) {
			qrstream_t *qrs = new_qrstream((qr_version_t)i, level);
			if (qrstream_try_for_string(qrs, src)) return qrs;
			qrstream_free(qrs);
		}
	}

	qrstream_t *qrs = new_qrstream(version, level);
	qrstream_try_for_string(qrs, src);
	return qrs;
#else
	return NULL;
#endif
}

qrstream_t create_qrstream_for_string(qr_version_t version, qr_errorlevel_t level, const char *src) {
	if (version == QR_VERSION_AUTO) {
		for (int i = QR_VERSION_1; i <= QR_VERSION_40; i++) {
			qrstream_t qrs = create_qrstream((qr_version_t)i, level);
			if (qrstream_try_for_string(&qrs, src)) return qrs;
			qrstream_destroy(&qrs);
		}
	}

	qrstream_t qrs = create_qrstream(version, level);
	qrstream_try_for_string(&qrs, src);
	return qrs;
}

static bitpos_t qrstream_data_words_iter(bitstream_t *bs, bitpos_t i, void *opaque) {
	qrstream_t *qrs = (qrstream_t *)opaque;
	bitpos_t n = i / 8;
	bitpos_t u = i % 8;

	if (n >= qrs->data_words) return BITPOS_END;

	// data words
	if (n < qrs->data_words_in_small_block * qrs->small_blocks) {
		const bitpos_t x = n % qrs->data_words_in_small_block;
		const bitpos_t y = n / qrs->data_words_in_small_block;

		return (x * qrs->total_blocks + y) * 8 + u;
	} else {
		const bitpos_t base = qrs->data_words_in_small_block * qrs->small_blocks;
		n -= base;

		const bitpos_t x = n % qrs->data_words_in_large_block;
		const bitpos_t y = n / qrs->data_words_in_large_block;

		return (qrs->small_blocks * MIN(x + 1, qrs->data_words_in_small_block) + qrs->large_blocks * x + y) * 8 + u;
	}
}

static bitpos_t qrstream_error_words_iter(bitstream_t *bs, bitpos_t i, void *opaque) {
	qrstream_t *qrs = (qrstream_t *)opaque;
	bitpos_t n = i / 8;
	bitpos_t u = i % 8;

	if (n >= qrs->error_words) return BITPOS_END;

	// error words
	const bitpos_t x = n % qrs->error_words_in_block;
	const bitpos_t y = n / qrs->error_words_in_block;

	return (x * qrs->total_blocks + y + qrs->data_words) * 8 + u;
}

void qrstream_set_error_words(qrstream_t *qrs) {
	CREATE_GF2_POLY(g, qrs->error_words_in_block);
	rs_init_generator_polynomial(g);

	bitstream_t bs_data = qrstream_read_bitstream_for_data(qrs);
	bitstream_t bs_error = qrstream_read_bitstream_for_error(qrs);

	for (bitpos_t rsblock_num = 0; rsblock_num < qrs->total_blocks; rsblock_num++) {
		bitpos_t error_words = qrs->error_words_in_block;
		bitpos_t data_words = rsblock_num < qrs->small_blocks ? qrs->data_words_in_small_block : qrs->data_words_in_large_block;

		// I
		CREATE_GF2_POLY(I, data_words + error_words - 1);
		for (bitpos_t i = 0; i < data_words; i++) {
			uint8_t data = bitstream_read_bits(&bs_data, 8);
			GF2_POLY_COEFF(I, error_words + data_words - i - 1) = data;
		}

		// calc parity
		CREATE_GF2_POLY(parity, data_words + error_words - 1);
		rs_calc_parity(parity, I, g);

		// write parity
		for (bitpos_t i = 0; i < error_words; i++) {
			bitstream_write_bits(&bs_error, GF2_POLY_COEFF(parity, error_words - i - 1), 8);
		}
	}
}

int qrstream_fix_errors(qrstream_t *qrs) {
	bitstream_t bs_data = qrstream_read_bitstream_for_data(qrs);
	bitstream_t bs_error = qrstream_read_bitstream_for_error(qrs);

	int num_errors_total = 0;
	for (uint16_t rsblock_num = 0; rsblock_num < qrs->total_blocks; rsblock_num++) {
		bitpos_t error_words = qrs->error_words_in_block;
		bitpos_t data_words = rsblock_num < qrs->small_blocks ? qrs->data_words_in_small_block : qrs->data_words_in_large_block;

		// backup
		bitpos_t pos_data = bitstream_tell(&bs_data);
		bitpos_t pos_error = bitstream_tell(&bs_error);

		int codelen = data_words + error_words;

		// fill data
		CREATE_GF2_POLY(R, codelen - 1);
		for (bitpos_t i = 0; i < data_words; i++) {
			GF2_POLY_COEFF(R, codelen - i - 1) = bitstream_read_bits(&bs_data, 8);
		}
		for (bitpos_t i = 0; i < error_words; i++) {
			GF2_POLY_COEFF(R, error_words - i - 1) = bitstream_read_bits(&bs_error, 8);
		}

		// fix error
		int num_errors;
		switch (num_errors = rs_fix_errors(R, error_words)) {
		default:
			debug_printf(BANNER "rsblock #%d: %d error(s) fixed\n", rsblock_num, num_errors);
			num_errors_total += num_errors;
			break;
		case 0:
			debug_printf(BANNER "rsblock #%d: No error\n", rsblock_num);
			continue;
		case -1:
			debug_printf(BANNER "rsblock #%d: Too much errors\n", rsblock_num);
			continue;
		};

		// write back
		bitstream_seek(&bs_error, pos_error);
		bitstream_seek(&bs_data, pos_data);
		for (bitpos_t i = 0; i < data_words; i++) {
			bitstream_write_bits(&bs_data, GF2_POLY_COEFF(R, codelen - i - 1), 8);
		}
		for (bitpos_t i = 0; i < error_words; i++) {
			bitstream_write_bits(&bs_error, GF2_POLY_COEFF(R, error_words - i - 1), 8);
		}
	}

	return num_errors_total;
}

bitstream_t qrstream_read_bitstream(qrstream_t *qrs) {
	return create_bitstream(qrs->buffer, qrs->total_bits, NULL, NULL);
}
bitstream_t qrstream_read_bitstream_for_data(qrstream_t *qrs) {
	return create_bitstream(qrs->buffer, qrs->total_bits, qrstream_data_words_iter, qrs);
}
bitstream_t qrstream_read_bitstream_for_error(qrstream_t *qrs) {
	return create_bitstream(qrs->buffer, qrs->total_bits, qrstream_error_words_iter, qrs);
}
