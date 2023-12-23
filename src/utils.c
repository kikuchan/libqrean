#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include "bitstream.h"
#include "utils.h"

padding_t create_padding1(uint8_t p) {
	return create_padding4(p, p, p, p);
}

padding_t create_padding2(uint8_t y, uint8_t x) {
	return create_padding4(y, x, y, x);
}

padding_t create_padding3(uint8_t u, uint8_t x, uint8_t b) {
	return create_padding4(u, x, b, x);
}

padding_t create_padding4(uint8_t u, uint8_t r, uint8_t b, uint8_t l) {
	padding_t p = {
		{u, r, b, l}
    };
	return p;
}

uint_fast8_t hamming_distance(uint32_t a, uint32_t b) {
	uint32_t c;
	bitpos_t d = 0;

	c = a ^ b;
	while (c) {
		c &= c - 1;
		d++;
	}

	return d;
}

uint_fast8_t hamming_distance_mem(const uint8_t *mem1, const uint8_t *mem2, bitpos_t bitlen) {
	int score = 0;
	for (bitpos_t i = 0; i < bitlen / 8; i++) {
		score += hamming_distance(mem1[i], mem2[i]);
	}
	if (bitlen % 8) {
		bitpos_t i = bitlen / 8;
		uint8_t mask = 0xff << (8 - bitlen % 8);
		score += hamming_distance(mem1[i] & mask, mem2[i] & mask);
	}
	return score;
}
