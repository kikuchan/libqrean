#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include "bitstream.h"
#include "utils.h"

padding_t create_padding1(uint8_t p)
{
	return create_padding4(p, p, p, p);
}

padding_t create_padding2(uint8_t y, uint8_t x)
{
	return create_padding4(y, x, y, x);
}

padding_t create_padding3(uint8_t u, uint8_t x, uint8_t b)
{
	return create_padding4(u, x, b, x);
}

padding_t create_padding4(uint8_t u, uint8_t r, uint8_t b, uint8_t l)
{
	padding_t p = {
		{u, r, b, l}
	};
	return p;
}

uint_fast8_t hamming_distance(uint32_t a, uint32_t b)
{
	uint32_t c;
	bitpos_t d = 0;

	c = a ^ b;
	while (c) {
		c &= c - 1;
		d++;
	}

	return d;
}

uint_fast8_t hamming_distance_mem(const uint8_t *mem1, const uint8_t *mem2, bitpos_t bitlen)
{
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

int calc_pattern_mismatch_error_rate(bitstream_t *bs, const void *pattern, bitpos_t size, bitpos_t start_idx, bitpos_t n)
{
	bitpos_t error = 0;
	bitpos_t total = 0;

	uint8_t buf[BYTE_SIZE(size)];
	bitpos_t e = start_idx + n;

	// cannot seek, so iterate over
	for (bitpos_t i = 0; !bitstream_is_end(bs) && (!n || i < e); i++) {
		bitpos_t l = bitstream_read(bs, buf, size, 1);
		if (!n || (start_idx <= i && i < e)) {
			error += hamming_distance_mem((const uint8_t *)buf, (const uint8_t *)pattern, l);
			total += l;
		}
	}

	return total ? error * 100 / total : 0;
}
