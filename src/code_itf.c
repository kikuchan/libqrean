#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitstream.h"
#include "qrean.h"

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

bitpos_t qrean_write_itf_string(qrean_t *qrean, const void *buf, size_t len, qrean_data_type_t data_type)
{
	const char *src = (const char *)buf;

	if (len % 2 != 0) {
		return 0;
	}
	for (int i = 0; i < len; i++) {
		if (src[i] < '0' || src[i] > '9') return 0;
	}

	bitstream_t bs = qrean_create_bitstream(qrean, NULL);

	// for animation, symbol size should be determined first
	uint8_t symbol_width = 4 + 9 * (len / 2) + 6;
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

	return symbol_width;
}

qrean_code_t qrean_code_itf = {
	.type = QREAN_CODE_TYPE_ITF,

	.write_data = qrean_write_itf_string,

	.init = NULL,
};
