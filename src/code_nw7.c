#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitstream.h"
#include "qrean.h"

static const struct {
	uint32_t v;
	int w;
} symbol[] = {
	{   /*  0 */ 0b10101000111, 11}, // 0
	{   /*  1 */ 0b10101110001, 11}, // 1
	{   /*  2 */ 0b10100010111, 11}, // 2
	{   /*  3 */ 0b11100010101, 11}, // 3
	{   /*  4 */ 0b10111010001, 11}, // 4
	{   /*  5 */ 0b11101010001, 11}, // 5
	{   /*  6 */ 0b10001010111, 11}, // 6
	{   /*  7 */ 0b10001011101, 11}, // 7
	{   /*  8 */ 0b10001110101, 11}, // 8
	{   /*  9 */ 0b11101000101, 11}, // 9
	{   /* 10 */ 0b10100011101, 11}, // -
	{   /* 11 */ 0b10111000101, 11}, // $
	{/*  12 */ 0b1110111011101, 13}, // .
	{/*  13 */ 0b1110111010111, 13}, // /
	{/*  14 */ 0b1110101110111, 13}, // :
	{/*  15 */ 0b1011101110111, 13}, // +

	{/*  16 */ 0b1011100010001, 13}, // A
	{/*  17 */ 0b1000100010111, 13}, // B
	{/*  18 */ 0b1010001000111, 13}, // C
	{/*  19 */ 0b1010001110001, 13}, // D
};

static const char *symbol_lookup = "0123456789-$./:+";

bitpos_t qrean_write_nw7_string(qrean_t *qrean, const void *buf, size_t len, qrean_data_type_t data_type)
{
	const char *src = (const char *)buf;
	int start_code, stop_code;
	int bitlen = 0;

	// check first letter
	if (*src == 'A' || *src == 'B' || *src == 'C' || *src == 'D') {
		start_code = *src++ - 'A' + 16;
	} else {
		start_code = 16; // default to 'A'
	}

	// check last letter
	if (src[len - 1] == 'A' || src[len - 1] == 'B' || src[len - 1] == 'C' || src[len - 1] == 'D') {
		stop_code = src[len - 1] - 'A' + 16;
		len--;
	} else {
		stop_code = 16; // default to 'A'
	}

	bitstream_t bs = qrean_create_bitstream(qrean, NULL);

	// --- for animation, symbol size should be determined first
	for (const char *p = src; p < src + len; p++) {
		const char *q = strchr(symbol_lookup, *p);
		if (!q) return 0;

		int n = q - symbol_lookup;
		bitlen += symbol[n].w + 1;
	}
	uint8_t symbol_width = symbol[start_code].w + 1 + bitlen + symbol[stop_code].w;
	qrean_set_symbol_width(qrean, symbol_width);
	// ---

	bitstream_write_bits(&bs, symbol[start_code].v, symbol[start_code].w); // start symbol
	bitstream_write_bits(&bs, 0, 1);

	for (const char *p = src; p < src + len; p++) {
		const char *q = strchr(symbol_lookup, *p);
		if (!q) return 0;

		int n = q - symbol_lookup;
		bitstream_write_bits(&bs, symbol[n].v, symbol[n].w);
		bitstream_write_bits(&bs, 0, 1);
	}

	bitstream_write_bits(&bs, symbol[stop_code].v, symbol[stop_code].w); // stop symbol

	return symbol_width;
}

qrean_code_t qrean_code_nw7 = {
	.type = QREAN_CODE_TYPE_NW7,
	.write_data = qrean_write_nw7_string,
};
