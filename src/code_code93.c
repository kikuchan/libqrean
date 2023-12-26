#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitstream.h"
#include "qrean.h"

static uint16_t symbol[] = {
	/*  0 */ 0b100010100, // 0
	/*  1 */ 0b101001000, // 1
	/*  2 */ 0b101000100, // 2
	/*  3 */ 0b101000010, // 3
	/*  4 */ 0b100101000, // 4
	/*  5 */ 0b100100100, // 5
	/*  6 */ 0b100100010, // 6
	/*  7 */ 0b101010000, // 7
	/*  8 */ 0b100010010, // 8
	/*  9 */ 0b100001010, // 9
	/* 10 */ 0b110101000, // A
	/* 11 */ 0b110100100, // B
	/* 12 */ 0b110100010, // C
	/* 13 */ 0b110010100, // D
	/* 14 */ 0b110010010, // E
	/* 15 */ 0b110001010, // F
	/* 16 */ 0b101101000, // G
	/* 17 */ 0b101100100, // H
	/* 18 */ 0b101100010, // I
	/* 19 */ 0b100110100, // J
	/* 20 */ 0b100011010, // K
	/* 21 */ 0b101011000, // L
	/* 22 */ 0b101001100, // M
	/* 23 */ 0b101000110, // N
	/* 24 */ 0b100101100, // O
	/* 25 */ 0b100010110, // P
	/* 26 */ 0b110110100, // Q
	/* 27 */ 0b110110010, // R
	/* 28 */ 0b110101100, // S
	/* 29 */ 0b110100110, // T
	/* 30 */ 0b110010110, // U
	/* 31 */ 0b110011010, // V
	/* 32 */ 0b101101100, // W
	/* 33 */ 0b101100110, // X
	/* 34 */ 0b100110110, // Y
	/* 35 */ 0b100111010, // Z
	/* 36 */ 0b100101110, // -
	/* 37 */ 0b111010100, // .
	/* 38 */ 0b111010010, // [SPACE]
	/* 39 */ 0b111001010, // $
	/* 40 */ 0b101101110, // /
	/* 41 */ 0b101110110, // +
	/* 42 */ 0b110101110, // %
	/* 43 */ 0b100100110, // ($)
	/* 44 */ 0b111011010, // (%)
	/* 45 */ 0b111010110, // (/)
	/* 46 */ 0b100110010, // (+)
	/* 47 */ 0b101011110, // [START] / [STOP]
	/* 48 */ 0b101111010, // (reverse stop)
	/* 49 */ 0b111101010, // (unused)
	/* 50 */ 0b101011100, // (unused)
	/* 51 */ 0b101001110, // (unused)
	/* 52 */ 0b101110100, // (unused)
	/* 53 */ 0b101110010, // (unused)
	/* 54 */ 0b110111010, // (unused)
	/* 55 */ 0b110110110, // (unused)
};

static const char *symbol_lookup = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ-. $/+%";

bitpos_t qrean_write_code93_string(qrean_t *qrean, const void *buf, size_t len, qrean_data_type_t data_type)
{
	const char *src = (const char *)buf;

	bitstream_t bs = qrean_create_bitstream(qrean, NULL);

	// for animation, symbol size should be determined first
	uint8_t symbol_width = 9 + 9 * len + 9 + 9 + 9 + 1;
	qrean_set_symbol_width(qrean, symbol_width);

	bitstream_write_bits(&bs, symbol[47], 9); // Start Symbol

	int w = (len - 1);
	int C = 0, K = 0;
	for (const char *p = src; *p; p++) {
		const char *q = strchr(symbol_lookup, *p);
		if (!q) return 0; // invalid symbol

		int n = q - symbol_lookup;

		C = (C + n * ((w + 0) % 20 + 1)) % 47;
		K = (K + n * ((w + 1) % 15 + 1)) % 47;

		w--;

		bitstream_write_bits(&bs, symbol[n], 9);
	}

	bitstream_write_bits(&bs, symbol[C], 9); // 1st check character
	K = (K + C) % 47;

	bitstream_write_bits(&bs, symbol[K], 9); // 2nd check character

	bitstream_write_bits(&bs, symbol[47], 9); // Stop Symbol
	bitstream_write_bits(&bs, 1, 1); // Termination bar

	return symbol_width;
}

qrean_code_t qrean_code_code93 = {
	.type = QREAN_CODE_TYPE_CODE93,

	.write_data = qrean_write_code93_string,

	.init = NULL,
};
