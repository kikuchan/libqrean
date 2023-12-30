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
	0b110100,
	0b101100,
	0b011100,
	0b110010,
	0b100110,
	0b001110,
	0b101010,
	0b011010,
	0b010110,
};

static uint8_t bit_reverse8(uint8_t v)
{
	v = (v & 0xf0) >> 4 | (v & 0x0f) << 4;
	v = (v & 0xcc) >> 2 | (v & 0x33) << 2;
	v = (v & 0xaa) >> 1 | (v & 0x55) << 1;
	return v;
}

static int calc_checkdigit(const char *src, size_t len)
{
	int result = 0;
	for (size_t i = 0; i < len; i++) {
		int v = src[len - i - 1] - '0';
		result = (result + (i % 2 ? v : v * 3)) % 10;
	}
	return 10 - result;
}

#define L_CODE (1 << 7)
#define G_CODE (1 << 6)
#define R_CODE (1 << 5)
#define MASK   (0x0f)

uint8_t lookup_symbol(uint8_t code)
{
	for (size_t i = 0; i < sizeof(symbol); i++) {
		if (symbol[i] == code) return i | L_CODE;
		if (((~bit_reverse8(symbol[i]) >> 1) & 0x7F) == code) return i | G_CODE;
		if ((~symbol[i] & 0x7F) == code) return i | R_CODE;
	}
	return 10;
}

size_t qrean_read_ean13_like_string(qrean_t *qrean, void *buffer, size_t size)
{
	char *dst = (char *)buffer;
	bitstream_t bs = qrean_create_bitstream(qrean, NULL);
	uint8_t tfd_parity = 0;

	// check start marker
	if (bitstream_read_bits(&bs, 3) != 0b101) return 0;

	int separator_found = 0;
	size_t i = 0;

	if (qrean->code->type == QREAN_CODE_TYPE_EAN13) {
		dst[i++] = '0';
	}
	while (i < size - 1) {
		// end marker
		if ((i == 13 && qrean->code->type == QREAN_CODE_TYPE_EAN13) || ((i == 12 && qrean->code->type == QREAN_CODE_TYPE_UPCA))
			|| ((i == 8 && qrean->code->type == QREAN_CODE_TYPE_EAN8))) {
			if (bitstream_read_bits(&bs, 3) != 0b101) return 0;
			break;
		}

		if ((i == 7 && qrean->code->type == QREAN_CODE_TYPE_EAN13) || ((i == 6 && qrean->code->type == QREAN_CODE_TYPE_UPCA))
			|| ((i == 4 && qrean->code->type == QREAN_CODE_TYPE_EAN8))) {
			if (bitstream_read_bits(&bs, 5) != 0b01010) return 0;
			separator_found = 1;
		}

		uint8_t sym = lookup_symbol(bitstream_read_bits(&bs, 7));
		if (sym == 10) return 0; // invalid

		dst[i++] = (sym & MASK) + '0';
		if (!separator_found) {
			tfd_parity = (tfd_parity << 1) | (sym & G_CODE ? 1 : 0);
			if (sym & R_CODE) return 0;
		} else if (!(sym & R_CODE)) {
			return 0;
		}
	}
	if (i >= size) i = size - 1;
	dst[i] = 0;

	if (qrean->code->type == QREAN_CODE_TYPE_EAN13) {
		tfd_parity = (bit_reverse8(tfd_parity) >> 2) & 0x3F;
		size_t j;
		for (j = 0; j < sizeof(parity); j++) {
			if (tfd_parity == parity[j]) {
				dst[0] = j + '0';
				break;
			}
		}
		if (j >= sizeof(parity)) return 0;
	} else if (tfd_parity) {
		return 0;
	}

	// sanity check
	if (i == 13 && qrean->code->type == QREAN_CODE_TYPE_EAN13) return i;
	if (i == 12 && qrean->code->type == QREAN_CODE_TYPE_UPCA) return i;
	if (i == 8 && qrean->code->type == QREAN_CODE_TYPE_EAN8) return i;
	return 0;
}

size_t qrean_write_ean13_like_string(qrean_t *qrean, const void *buf, size_t len, qrean_data_type_t data_type)
{
	const char *src = (const char *)buf;
	int tfd = 0;
	int checkdigit;
	int datasize_half;

	// sanity checks
	for (size_t i = 0; i < len; i++) {
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
		if (!(parity[tfd] & (1 << i))) {
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

	return len;
}

qrean_code_t qrean_code_ean13 = {
	.type = QREAN_CODE_TYPE_EAN13,
	.write_data = qrean_write_ean13_like_string,
	.read_data = qrean_read_ean13_like_string,
};

qrean_code_t qrean_code_ean8 = {
	.type = QREAN_CODE_TYPE_EAN8,
	.write_data = qrean_write_ean13_like_string,
	.read_data = qrean_read_ean13_like_string,
};
qrean_code_t qrean_code_upca = {
	.type = QREAN_CODE_TYPE_UPCA,
	.write_data = qrean_write_ean13_like_string,
	.read_data = qrean_read_ean13_like_string,
};
