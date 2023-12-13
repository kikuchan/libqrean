#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "barcode.h"
#include "bitstream.h"

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

void barcode_nw7_init(barcode_t *code) {
	barcode_buffer_init(code, 0);
}

void barcode_nw7_deinit(barcode_t *code) {
	barcode_buffer_deinit(code);
}

bitpos_t barcode_nw7_write_string(barcode_t *code, const char *src) {
	char tmp[strlen(src) + 1];
	char *p = tmp;
	int start_code, stop_code;

	// check first letter
	if (*p == 'A' || *p == 'B' || *p == 'C' || *p == 'D') {
		start_code = *p++ - 'A' + 16;
	} else {
		start_code = 16; // default to 'A'
	}

	// check last letter
	int len = strlen(p);
	if (p[len - 1] == 'A' || p[len - 1] == 'B' || p[len - 1] == 'C' || p[len - 1] == 'D') {
		stop_code = p[len - 1] - 'A' + 16;
		p[len - 1] = '\0'; // terminate
	} else {
		stop_code = 16; // default to 'A'
	}

	bitstream_t bs = barcode_create_bitstream(code, NULL);

	bitstream_write_bits(&bs, 0, 10);
	bitstream_write_bits(&bs, symbol[start_code].v, symbol[start_code].w); // start symbol
	bitstream_write_bits(&bs, 0, 1);

	for (; *p; p++) {
		const char *q = strchr(symbol_lookup, *p);
		if (!q) continue;

		int n = q - symbol_lookup;
		bitstream_write_bits(&bs, symbol[n].v, symbol[n].w);
		bitstream_write_bits(&bs, 0, 1);
	}

	bitstream_write_bits(&bs, symbol[stop_code].v, symbol[stop_code].w); // stop symbol
	bitstream_write_bits(&bs, 0, 10);

	barcode_set_size(code, bitstream_tell(&bs));
	return code->size;
}
