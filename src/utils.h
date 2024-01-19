#ifndef __QR_UTILS_H__
#define __QR_UTILS_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bitstream.h"

typedef union {
	struct {
		uint8_t t;
		uint8_t r;
		uint8_t b;
		uint8_t l;
	};
	uint8_t padding[4];
} padding_t;

padding_t create_padding1(uint8_t p);
padding_t create_padding2(uint8_t y, uint8_t x);
padding_t create_padding3(uint8_t t, uint8_t x, uint8_t b);
padding_t create_padding4(uint8_t t, uint8_t r, uint8_t b, uint8_t l);

uint_fast8_t hamming_distance(uint32_t a, uint32_t b);
uint_fast8_t hamming_distance_mem(const uint8_t *mem1, const uint8_t *mem2, bitpos_t bitlen);

int calc_pattern_mismatch_error_rate(bitstream_t *bs, const void *pattern, bitpos_t size, bitpos_t start_idx, bitpos_t n);

int safe_fprintf(FILE *fp, const char *fmt, ...);

#define READ_BIT(buffer, pos) (((uint8_t *)(buffer))[(pos) / 8] & (0x80 >> ((pos) % 8)) ? 1 : 0)
#define WRITE_BIT(buffer, pos, v)                          \
	do {                                                   \
		if ((v)) {                                         \
			(buffer)[(pos) / 8] |= (0x80 >> ((pos) % 8));  \
		} else {                                           \
			(buffer)[(pos) / 8] &= ~(0x80 >> ((pos) % 8)); \
		}                                                  \
	} while (0)

#define BYTE_SIZE(bits) (((bits) + 7) / 8)

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define UNUSED(x) ((void)(x))

#define QREAN_XY_TO_BITPOS(qrean, x, y)                                                                  \
	(((x) < 0 || (y) < 0 || (x) >= (qrean)->canvas.symbol_width || (y) >= (qrean)->canvas.symbol_height) \
			? BITPOS_BLANK                                                                               \
			: (((y) * (qrean)->canvas.stride + (x)) & BITPOS_MASK))

#define QREAN_XYV_TO_BITPOS(qrean, x, y, v) (QREAN_XY_TO_BITPOS(qrean, x, y) | ((v) ? BITPOS_TOGGLE : 0))

#endif /* __QR_UTILS_H__ */
