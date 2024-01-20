#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitstream.h"
#include "qrean.h"
#include "runlength.h"

static const uint8_t symbol[] = {
	0b00110,
	0b10001,
	0b01001,
	0b11000,
	0b00101,
	0b10100,
	0b01100,
	0b00011,
	0b10010,
	0b01010,
};

static int8_t lookup_symbol(uint8_t sym)
{
	for (size_t i = 0; i < sizeof(symbol); i++) {
		if (sym == symbol[i]) return i;
	}
	return -1;
}

size_t qrean_read_itf_string(qrean_t *qrean, void *buf, size_t size)
{
	char *dst = (char *)buf;
	bitstream_t bs = qrean_create_bitstream(qrean, NULL);

	if (bitstream_read_bits(&bs, 4) != 0b1010) return 0;

	size_t len = 0;
	while (len < size - 1) {
		runlength_t rl = create_runlength();

		for (int i = 0; i < 9 * 2; i++) {
			runlength_push_value(&rl, bitstream_read_bit(&bs));
			if (runlength_get_count(&rl, 0) > 3) break;
		}
		if (runlength_get_count(&rl, 0) > 3) break;

		uint8_t sym1 = 0;
		uint8_t sym2 = 0;
		for (int i = 10 - 1; i >= 1; i -= 2) {
			runlength_count_t l1 = runlength_get_count(&rl, i);
			sym1 = (sym1 << 1) | (l1 == 3 ? 1 : 0);
			runlength_count_t l2 = runlength_get_count(&rl, i - 1);
			sym2 = (sym2 << 1) | (l2 == 3 ? 1 : 0);
		}
		int8_t ch1 = lookup_symbol(sym1);
		int8_t ch2 = lookup_symbol(sym2);

		if (ch1 < 0 || ch2 < 0) return 0;

		dst[len++] = ch1 + '0';
		dst[len++] = ch2 + '0';
	}
	dst[len] = '\0';

	uint8_t symbol_width = 4 + 9 * len + 6;
	qrean_set_symbol_width(qrean, symbol_width);
	return len;
}

size_t qrean_write_itf_string(qrean_t *qrean, const void *buf, size_t len, qrean_data_type_t data_type)
{
	const char *src = (const char *)buf;

	if (len % 2 != 0) {
		return 0;
	}
	for (size_t i = 0; i < len; i++) {
		if (src[i] < '0' || src[i] > '9') return 0;
	}

	bitstream_t bs = qrean_create_bitstream(qrean, NULL);

	// for animation, symbol size should be determined first
	uint8_t symbol_width = 4 + 9 * len + 6;
	qrean_set_symbol_width(qrean, symbol_width);

	bitstream_write_bits(&bs, 0b1010, 4);

	for (const char *p = src; *p; p += 2) {
		int on = p[0] - '0'; // odd number
		int en = p[1] - '0'; // even number

		for (int i = 0; i < 5; i++) {
			bitstream_write_bits(&bs, 0xff, symbol[on] & (1 << (4 - i)) ? 3 : 1);
			bitstream_write_bits(&bs, 0x00, symbol[en] & (1 << (4 - i)) ? 3 : 1);
		}
	}

	bitstream_write_bits(&bs, 0b111010, 6);

	return len;
}

qrean_code_t qrean_code_itf = {
	.type = QREAN_CODE_TYPE_ITF,
	.write_data = qrean_write_itf_string,
	.read_data = qrean_read_itf_string,
	.padding = { { 4, 10, 4, 10 } },
};
