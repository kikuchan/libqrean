#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitstream.h"
#include "hexdump.h"

#define BYTE_SIZE(len) (((len) + 7) / 8)

void bitstream_init(bitstream_t *bs, void *src, bitpos_t len, bitstream_iterator_t iter, void *opaque) {
	assert(src != NULL);

	bs->size = len;
	bs->bits = (uint8_t *)src;
	bs->pos = 0;

	bs->iter = iter;
	bs->opaque = opaque;

#ifndef NO_CALLBACK
	bs->get_bit_at = NULL;
	bs->put_bit_at = NULL;
#endif
}

bitstream_t create_bitstream(const void *src, bitpos_t len, bitstream_iterator_t iter, void *opaque) {
	bitstream_t bs;
	bitstream_init(&bs, (void *)src, len, iter, opaque);
	return bs;
}

bitpos_t bitstream_length(bitstream_t *bs) {
	return bs->size;
}

void bitstream_fill(bitstream_t *bs, bit_t v) {
	for (bitpos_t i = 0; i < bs->size; i++) {
		bitstream_put_bit(bs, v);
	}
}

void bitstream_seek(bitstream_t *bs, bitpos_t pos) {
	bs->pos = pos;
}
bitpos_t bitstream_tell(bitstream_t *bs) {
	return bs->pos;
}
void bitstream_rewind(bitstream_t *bs) {
	bitstream_seek(bs, 0);
}

static bitpos_t get_iter_pos(bitstream_t *bs) {
	bitpos_t pos = bs->iter ? bs->iter(bs, bs->pos, bs->opaque) : bs->pos;
	if (pos == bs->size) return BITPOS_END;
	return pos;
}

bit_t bitstream_is_end(bitstream_t *bs) {
	bitpos_t pos = get_iter_pos(bs);
	if (pos == BITPOS_END) return 1;
	return 0;
}

static bit_t bitstream_get_bit_at(bitstream_t *bs, bitpos_t pos) {
	if (pos >= bs->size) return 0;

#ifndef NO_CALLBACK
	if (bs->get_bit_at) {
		return bs->get_bit_at(bs, pos, bs->opaque_get);
	}
#endif

	return bs->bits[pos / 8] & (0x80 >> (pos % 8)) ? 1 : 0;
}

static int bitstream_put_bit_at(bitstream_t *bs, bitpos_t pos, bit_t bit) {
	if (pos >= bs->size) return 0;

#ifndef NO_CALLBACK
	if (bs->put_bit_at) {
		bs->put_bit_at(bs, pos, bs->opaque_put, bit);
	} else
#endif
	{
		if (bit) {
			bs->bits[pos / 8] |= (0x80 >> (pos % 8));
		} else {
			bs->bits[pos / 8] &= ~(0x80 >> (pos % 8));
		}
	}

	return 1;
}

bit_t bitstream_get_bit(bitstream_t *bs) {
	bitpos_t pos;
	do {
		pos = get_iter_pos(bs);
		if (pos == BITPOS_END) return 0;
		bs->pos++;
	} while (pos == BITPOS_TRUNC);
	if (pos == BITPOS_BLANK) return 0;

	return bitstream_get_bit_at(bs, pos & 0x7FFFFFFF) ^ ((pos & 0x80000000) ? 1 : 0);
}

uint_fast32_t bitstream_get_bits(bitstream_t *bs, uint_fast8_t num_bits) {
	uint_fast32_t val = 0;

	assert(1 <= num_bits && num_bits <= 32);

	for (uint_fast8_t i = 0; i < num_bits; i++) {
		val = (val << 1) | bitstream_get_bit(bs);
	}
	return val;
}

int bitstream_put_bit(bitstream_t *bs, bit_t bit) {
	bitpos_t pos;

	do {
		pos = get_iter_pos(bs);
		if (pos == BITPOS_END) return 0; // EOF
		bs->pos++;
	} while (pos == BITPOS_TRUNC);

	if (pos == BITPOS_BLANK) return 1;
	return bitstream_put_bit_at(bs, (pos & 0x7FFFFFFF), ((bit ? 1 : 0) ^ ((pos & 0x80000000) ? 1 : 0)));
}

int bitstream_put_bits(bitstream_t *bs, uint_fast32_t value, uint_fast8_t num_bits) {
	assert(0 <= num_bits && num_bits <= 32);

	for (uint_fast8_t i = 0; i < num_bits; i++) {
		if (!bitstream_put_bit(bs, (value & (1 << (num_bits - 1 - i))) ? 1 : 0)) {
			return 0;
		}
	}
	return 1;
}

bitpos_t bitstream_copy(bitstream_t *dst, bitstream_t *src, bitpos_t size, bitpos_t n) {
	bitpos_t i;
	for (i = 0; !size || !n || i < size * n; i++) {
		if (bitstream_is_end(src) || bitstream_is_end(dst)) break;
		if (!bitstream_put_bit(dst, bitstream_get_bit(src))) break;
	}
	return i;
}

bitpos_t bitstream_write(bitstream_t *dst, const void *buffer, bitpos_t size, bitpos_t n) {
	bitstream_t src = create_bitstream(buffer, size, bitstream_loop_iter, NULL);
	return bitstream_copy(dst, &src, size, n);
}

bitpos_t bitstream_read(bitstream_t *src, void *buffer, bitpos_t size, bitpos_t n) {
	bitstream_t dst = create_bitstream(buffer, size, bitstream_loop_iter, NULL);
	return bitstream_copy(&dst, src, size, n);
}

// DEBUG
#ifndef NO_PRINTF
void bitstream_dump(bitstream_t *bs) {
	hexdump(bs->bits, BYTE_SIZE(bs->size), 0);
}
#endif

bitpos_t bitstream_loop_iter(bitstream_t *bs, bitpos_t i, void *opaque) {
	return i % bs->size;
}

void bitstream_on_put_bit(bitstream_t *bs, bitstream_put_bit_callback_t cb, void *opaque) {
#ifndef NO_CALLBACK
	bs->put_bit_at = cb;
	bs->opaque_put = opaque;
#endif
}

void bitstream_on_get_bit(bitstream_t *bs, bitstream_get_bit_callback_t cb, void *opaque) {
#ifndef NO_CALLBACK
	bs->get_bit_at = cb;
	bs->opaque_get = opaque;
#endif
}
