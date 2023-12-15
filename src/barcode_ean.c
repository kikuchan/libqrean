#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "barcode.h"

// https://en.wikipedia.org/wiki/International_Article_Number

static const uint8_t symbol[] = {
	0b0001101, 0b0011001, 0b0010011, 0b0111101, 0b0100011, 0b0110001, 0b0101111, 0b0111011, 0b0110111, 0b0001011,
};

static const uint8_t parity[] = {
	0b000000, 0b001011, 0b001101, 0b001110, 0b010011, 0b011001, 0b011100, 0b010101, 0b010110, 0b011010,
};

static uint8_t bit_reverse8(uint8_t v) {
	v = (v & 0xf0) >> 4 | (v & 0x0f) << 4;
	v = (v & 0xcc) >> 2 | (v & 0x33) << 2;
	v = (v & 0xaa) >> 1 | (v & 0x55) << 1;
	return v;
}

static int calc_checkdigit(const char *src, int len) {
	int result = 0;
	for (int i = 0; i < len; i++) {
		int v = src[len - i - 1] - '0';
		result = (result + (i % 2 ? v : v * 3)) % 10;
	}
	return 10 - result;
}

bitpos_t barcode_write_ean13_like_string(barcode_t *code, barcode_type_t type, const char *src) {
	int tfd = 0;
	int len = strlen(src);
	int checkdigit;
	int datasize_half;

	// sanity checks
	for (int i = 0; i < len; i++) {
		if (src[i] < '0' || src[i] > '9') return 0;
	}

	switch (type) {
	case BARCODE_TYPE_UPCA:
		if (len == 12)
			checkdigit = src[len - 1] - '0';
		else if (len == 11)
			checkdigit = calc_checkdigit(src, len);
		else
			return 0;
		datasize_half = 6;
		break;
	case BARCODE_TYPE_EAN13:
		if (len == 13)
			checkdigit = src[len - 1] - '0';
		else if (len == 12)
			checkdigit = calc_checkdigit(src, len);
		else
			return 0;
		datasize_half = 6;
		break;
	case BARCODE_TYPE_EAN8:
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

	bitstream_t bs = barcode_create_bitstream(code, NULL);

	// start marker
	bitstream_write_bits(&bs, 0b101, 3);

	const char *ptr = src;
	// the first digit
	if (type == BARCODE_TYPE_UPCA || type == BARCODE_TYPE_EAN8) {
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

	barcode_set_size(code, bitstream_tell(&bs));
	return code->size;
}

bitpos_t barcode_write_ean13_string(barcode_t *code, const char *src) {
	return barcode_write_ean13_like_string(code, BARCODE_TYPE_EAN13, src);
}

bitpos_t barcode_write_ean8_string(barcode_t *code, const char *src) {
	return barcode_write_ean13_like_string(code, BARCODE_TYPE_EAN8, src);
}
bitpos_t barcode_write_upca_string(barcode_t *code, const char *src) {
	return barcode_write_ean13_like_string(code, BARCODE_TYPE_UPCA, src);
}
