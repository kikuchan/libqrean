#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitstream.h"
#include "qrean.h"

// https://en.wikipedia.org/wiki/International_Article_Number

static const uint8_t symbol[] = {
	0b0001101,
	0b0011001,
	0b0010011,
	0b0111101,
	0b0100011,
	0b0110001,
	0b0101111,
	0b0111011,
	0b0110111,
	0b0001011,
};

static const uint8_t parity[] = {
	0b000000,
	0b001011,
	0b001101,
	0b001110,
	0b010011,
	0b011001,
	0b011100,
	0b010101,
	0b010110,
	0b011010,
};

static uint8_t bit_reverse8(uint8_t v)
{
	v = (v & 0xf0) >> 4 | (v & 0x0f) << 4;
	v = (v & 0xcc) >> 2 | (v & 0x33) << 2;
	v = (v & 0xaa) >> 1 | (v & 0x55) << 1;
	return v;
}

static int calc_checkdigit(const char *src, int len)
{
	int result = 0;
	for (int i = 0; i < len; i++) {
		int v = src[len - i - 1] - '0';
		result = (result + (i % 2 ? v : v * 3)) % 10;
	}
	return 10 - result;
}

bitpos_t qrean_write_ean13_like_string(qrean_t *qrean, const void *buf, size_t len, qrean_data_type_t data_type)
{
	const char *src = (const char *)buf;
	int tfd = 0;
	int checkdigit;
	int datasize_half;

	// sanity checks
	for (int i = 0; i < len; i++) {
		if (src[i] < '0' || src[i] > '9') return 0;
	}

	switch (qrean->code->type) {
	case QREAN_CODE_TYPE_UPCA:
		if (len == 12)
			checkdigit = src[len - 1] - '0';
		else if (len == 11)
			checkdigit = calc_checkdigit(src, len);
		else
			return 0;
		datasize_half = 6;
		break;
	case QREAN_CODE_TYPE_EAN13:
		if (len == 13)
			checkdigit = src[len - 1] - '0';
		else if (len == 12)
			checkdigit = calc_checkdigit(src, len);
		else
			return 0;
		datasize_half = 6;
		break;
	case QREAN_CODE_TYPE_EAN8:
		if (len == 8)
			checkdigit = src[len - 1] - '0';
		else if (len == 7)
			checkdigit = calc_checkdigit(src, len);
		else
			return 0;
		datasize_half = 4;
		break;
	default:
		return 0;
	}

	bitstream_t bs = qrean_create_bitstream(qrean, NULL);

	// for animation, symbol size should be determined first
	uint8_t symbol_width = 3 + datasize_half * 7 * 2 + 5 + 3;
	qrean_set_symbol_width(qrean, symbol_width);

	// start marker
	bitstream_write_bits(&bs, 0b101, 3);

	const char *ptr = src;
	// the first digit
	if (qrean->code->type == QREAN_CODE_TYPE_UPCA || qrean->code->type == QREAN_CODE_TYPE_EAN8) {
		tfd = 0;
	} else {
		tfd = (*ptr++) - '0';
	}

	// L-code / G-code
	int i;
	for (i = 0; i < datasize_half; i++) {
		int n = (*ptr++) - '0';
		if (!(parity[tfd] & (0b100000 >> i))) {
			bitstream_write_bits(&bs, symbol[n], 7); // L-code
		} else {
			bitstream_write_bits(&bs, ~bit_reverse8(symbol[n]) >> 1, 7); // G-code
		}
	}

	// separator
	bitstream_write_bits(&bs, 0b01010, 5);

	// R-code
	for (i = 0; i < datasize_half; i++) {
		int n = *ptr ? (*ptr++ - '0') : checkdigit;
		bitstream_write_bits(&bs, ~symbol[n], 7);
	}

	// end marker
	bitstream_write_bits(&bs, 0b101, 3);

	return symbol_width;
}

qrean_code_t qrean_code_ean13 = {
	.type = QREAN_CODE_TYPE_EAN13,

	.write_data = qrean_write_ean13_like_string,

	.init = NULL,
};

qrean_code_t qrean_code_ean8 = {
	.type = QREAN_CODE_TYPE_EAN8,

	.write_data = qrean_write_ean13_like_string,

	.init = NULL,
};
qrean_code_t qrean_code_upca = {
	.type = QREAN_CODE_TYPE_UPCA,

	.write_data = qrean_write_ean13_like_string,

	.init = NULL,
};
