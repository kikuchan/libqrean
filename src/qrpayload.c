#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "bitstream.h"
#include "debug.h"
#include "galois.h"
#include "qrdata.h"
#include "qrpayload.h"
#include "qrspec.h"
#include "reedsolomon.h"
#include "utils.h"

#define QRSTREAM_BUFFER_SIZE(payload) (((payload)->total_bits + 7) / 8)

void qrpayload_init(qrpayload_t *payload, qr_version_t version, qr_errorlevel_t level) {
	assert(IS_QR(version) || IS_MQR(version));
	assert(QR_ERRORLEVEL_L <= level && level <= QR_ERRORLEVEL_H);

	payload->version = version;
	payload->level = level;

	// obtain spec info
	payload->total_bits = qrspec_get_available_bits(version);
	payload->total_blocks = qrspec_get_total_blocks(version, level);
	payload->error_words_in_block = qrspec_get_error_words_in_block(version, level);

	// let's do the simple math
	payload->total_words = payload->total_bits / 8;
	payload->error_words = payload->error_words_in_block * payload->total_blocks;
	payload->data_words = payload->total_words - payload->error_words;

	payload->large_blocks = payload->data_words % payload->total_blocks;
	payload->small_blocks = payload->total_blocks - payload->large_blocks;

	payload->data_words_in_small_block = payload->data_words / payload->total_blocks;
	payload->data_words_in_large_block = payload->data_words_in_small_block + 1;

	payload->error_words_in_block = payload->error_words / payload->total_blocks;

	payload->total_words_in_small_block = payload->data_words_in_small_block + payload->error_words_in_block;
	payload->total_words_in_large_block = payload->data_words_in_large_block + payload->error_words_in_block;

#if defined(USE_MALLOC_BUFFER) && !defined(NO_MALLOC)
	payload->buffer = (uint8_t *)malloc(QRSTREAM_BUFFER_SIZE(payload));
#endif
	memset(payload->buffer, 0, QRSTREAM_BUFFER_SIZE(payload));
}

void qrpayload_deinit(qrpayload_t *payload) {
#if defined(USE_MALLOC_BUFFER) && !defined(NO_MALLOC)
	free(payload->buffer);
#endif
}

qrpayload_t create_qrpayload(qr_version_t version, qr_errorlevel_t level) {
	qrpayload_t payload;
	qrpayload_init(&payload, version, level);
	return payload;
}

void qrpayload_destroy(qrpayload_t *payload) {
	qrpayload_deinit(payload);
}

qrpayload_t *new_qrpayload(qr_version_t version, qr_errorlevel_t level) {
#ifndef NO_MALLOC
	qrpayload_t *payload = (qrpayload_t *)malloc(sizeof(qrpayload_t));
	qrpayload_init(payload, version, level);
	return payload;
#else
	return NULL;
#endif
}

void qrpayload_free(qrpayload_t *payload) {
#ifndef NO_MALLOC
	free(payload);
#endif
}

static bitpos_t qrpayload_data_words_iter(bitstream_t *bs, bitpos_t i, void *opaque) {
	qrpayload_t *payload = (qrpayload_t *)opaque;
	bitpos_t n = i / 8;
	bitpos_t u = i % 8;

	if (n >= payload->data_words) return BITPOS_END;

	// data words
	if (n < payload->data_words_in_small_block * payload->small_blocks) {
		const bitpos_t x = n % payload->data_words_in_small_block;
		const bitpos_t y = n / payload->data_words_in_small_block;

		return (x * payload->total_blocks + y) * 8 + u;
	} else {
		const bitpos_t base = payload->data_words_in_small_block * payload->small_blocks;
		n -= base;

		const bitpos_t x = n % payload->data_words_in_large_block;
		const bitpos_t y = n / payload->data_words_in_large_block;

		return (payload->small_blocks * MIN(x + 1, payload->data_words_in_small_block) + payload->large_blocks * x + y) * 8 + u;
	}
}

static bitpos_t qrpayload_error_words_iter(bitstream_t *bs, bitpos_t i, void *opaque) {
	qrpayload_t *payload = (qrpayload_t *)opaque;
	bitpos_t n = i / 8;
	bitpos_t u = i % 8;

	if (n >= payload->error_words) return BITPOS_END;

	// error words
	const bitpos_t x = n % payload->error_words_in_block;
	const bitpos_t y = n / payload->error_words_in_block;

	return (x * payload->total_blocks + y + payload->data_words) * 8 + u;
}

void qrpayload_set_error_words(qrpayload_t *payload) {
	CREATE_GF2_POLY(g, payload->error_words_in_block);
	rs_init_generator_polynomial(g);

	bitstream_t bs_data = qrpayload_get_bitstream_for_data(payload);
	bitstream_t bs_error = qrpayload_get_bitstream_for_error(payload);

	for (bitpos_t rsblock_num = 0; rsblock_num < payload->total_blocks; rsblock_num++) {
		bitpos_t error_words = payload->error_words_in_block;
		bitpos_t data_words = rsblock_num < payload->small_blocks ? payload->data_words_in_small_block : payload->data_words_in_large_block;

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

int qrpayload_fix_errors(qrpayload_t *payload) {
	bitstream_t bs_data = qrpayload_get_bitstream_for_data(payload);
	bitstream_t bs_error = qrpayload_get_bitstream_for_error(payload);

	int num_errors_total = 0;
	for (uint16_t rsblock_num = 0; rsblock_num < payload->total_blocks; rsblock_num++) {
		bitpos_t error_words = payload->error_words_in_block;
		bitpos_t data_words = rsblock_num < payload->small_blocks ? payload->data_words_in_small_block : payload->data_words_in_large_block;

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
			qrean_debug_printf("rsblock #%d: %d error(s) fixed\n", rsblock_num, num_errors);
			num_errors_total += num_errors;
			break;
		case 0:
			qrean_debug_printf("rsblock #%d: No error\n", rsblock_num);
			continue;
		case -1:
			qrean_debug_printf("rsblock #%d: Too much errors\n", rsblock_num);
			return -1;
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

bitstream_t qrpayload_get_bitstream(qrpayload_t *payload) {
	return create_bitstream(payload->buffer, payload->total_bits, NULL, NULL);
}
bitstream_t qrpayload_get_bitstream_for_data(qrpayload_t *payload) {
	return create_bitstream(payload->buffer, payload->total_bits, qrpayload_data_words_iter, payload);
}
bitstream_t qrpayload_get_bitstream_for_error(qrpayload_t *payload) {
	return create_bitstream(payload->buffer, payload->total_bits, qrpayload_error_words_iter, payload);
}

static void qrdata_parse_on_letter(qr_data_mode_t mode, uint32_t letter, void *opaque) {
	bitstream_t *bs = (bitstream_t *)opaque;
	if (mode != QR_DATA_MODE_KANJI) {
		bitstream_write_bits(bs, letter, 8);
	}
}

size_t qrpayload_read_string(qrpayload_t *payload, char *buffer, size_t size) {
	qrdata_t qrdata = create_qrdata_for(qrpayload_get_bitstream_for_data(payload), payload->version);
	bitstream_t bs = create_bitstream(buffer, size * 8, NULL, NULL);
	size_t len = qrdata_parse(&qrdata, qrdata_parse_on_letter, &bs);
	if (len >= size) len = size - 1;
	buffer[len] = 0;
	return len;
}

bit_t qrpayload_write_string(qrpayload_t *payload, const char *src, size_t len, qrdata_writer_t writer) {
	qrdata_t data = create_qrdata_for(qrpayload_get_bitstream_for_data(payload), payload->version);
	if (writer(&data, src, len) == len && qrdata_finalize(&data)) {
		qrpayload_set_error_words(payload);
		return 1;
	}
	return 0;
}

void qrpayload_dump(qrpayload_t *payload, FILE *out) {
	bitstream_t bs = qrpayload_get_bitstream(payload);
	bitstream_dump(&bs, 0, out);
}
void qrpayload_dump_for_data(qrpayload_t *payload, FILE *out) {
	bitstream_t bs = qrpayload_get_bitstream_for_data(payload);
	bitstream_dump(&bs, 0, out);
}
void qrpayload_dump_for_error(qrpayload_t *payload, FILE *out) {
	bitstream_t bs = qrpayload_get_bitstream_for_error(payload);
	bitstream_dump(&bs, 0, out);
}
