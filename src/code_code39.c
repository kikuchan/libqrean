#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitstream.h"
#include "qrean.h"

static uint16_t symbol[] = {
	/*  0 */ 0b111010001010111, // 1
	/*  1 */ 0b101110001010111, // 2
	/*  2 */ 0b111011100010101, // 3
	/*  3 */ 0b101000111010111, // 4
	/*  4 */ 0b111010001110101, // 5
	/*  5 */ 0b101110001110101, // 6
	/*  6 */ 0b101000101110111, // 7
	/*  7 */ 0b111010001011101, // 8
	/*  8 */ 0b101110001011101, // 9
	/*  9 */ 0b101000111011101, // 0
	/* 10 */ 0b111010100010111, // A
	/* 11 */ 0b101110100010111, // B
	/* 12 */ 0b111011101000101, // C
	/* 13 */ 0b101011100010111, // D
	/* 14 */ 0b111010111000101, // E
	/* 15 */ 0b101110111000101, // F
	/* 16 */ 0b101010001110111, // G
	/* 17 */ 0b111010100011101, // H
	/* 18 */ 0b101110100011101, // I
	/* 19 */ 0b101011100011101, // J
	/* 20 */ 0b111010101000111, // K
	/* 21 */ 0b101110101000111, // L
	/* 22 */ 0b111011101010001, // M
	/* 23 */ 0b101011101000111, // N
	/* 24 */ 0b111010111010001, // O
	/* 25 */ 0b101110111010001, // P
	/* 26 */ 0b101010111000111, // Q
	/* 27 */ 0b111010101110001, // R
	/* 28 */ 0b101110101110001, // S
	/* 29 */ 0b101011101110001, // T
	/* 30 */ 0b111000101010111, // U
	/* 31 */ 0b100011101010111, // V
	/* 32 */ 0b111000111010101, // W
	/* 33 */ 0b100010111010111, // X
	/* 34 */ 0b111000101110101, // Y
	/* 35 */ 0b100011101110101, // Z
	/* 36 */ 0b100010101110111, // -
	/* 37 */ 0b111000101011101, // .
	/* 38 */ 0b100011101011101, //
	/* 39 */ 0b100010001000101, // $
	/* 40 */ 0b100010001010001, // /
	/* 41 */ 0b100010100010001, // +
	/* 42 */ 0b101000100010001, // %

	/* 43 */ 0b100010111011101, // *
};

static const char *symbol_lookup = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ-. $/+%";

bitpos_t qrean_write_code39_string(qrean_t *qrean, const void *buf, size_t len, qrean_data_type_t data_type)
{
	const char *src = (const char *)buf;
	bitstream_t bs = qrean_create_bitstream(qrean, NULL);

	// for animation, symbol size should be determined first
	uint8_t symbol_width = 15 + 1 + len * 16 + 15;
	qrean_set_symbol_width(qrean, symbol_width);

	bitstream_write_bits(&bs, symbol[43], 15); // Start Symbol
	bitstream_write_bits(&bs, 0, 1);

	for (const char *p = src; *p; p++) {
		const char *q = strchr(symbol_lookup, *p);
		if (!q) return 0; // Invalid code

		bitstream_write_bits(&bs, symbol[q - symbol_lookup], 15);
		bitstream_write_bits(&bs, 0, 1);
	}

	bitstream_write_bits(&bs, symbol[43], 15); // Stop Symbol

	return symbol_width;
}

qrean_code_t qrean_code_code39 = {
	.type = QREAN_CODE_TYPE_CODE39,
	.write_data = qrean_write_code39_string,
};
