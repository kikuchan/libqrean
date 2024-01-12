#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitstream.h"
#include "utils.h"

void bitstream_init(bitstream_t *bs, void *src, bitpos_t len, bitstream_iterator_t iter, void *opaque)
{
	assert(src != NULL);

	bs->size = len;
	bs->bits = (uint8_t *)src;
	bs->pos = 0;

	bs->iter = iter;
	bs->opaque = opaque;

#ifndef NO_CALLBACK
	bs->read_bit_at = NULL;
	bs->write_bit_at = NULL;
#endif
}

bitstream_t create_bitstream(const void *src, bitpos_t len, bitstream_iterator_t iter, void *opaque)
{
	bitstream_t bs;
	bitstream_init(&bs, (void *)src, len, iter, opaque);
	return bs;
}

bitpos_t bitstream_length(bitstream_t *bs)
{
	return bs->size;
}

void bitstream_fill(bitstream_t *bs, bit_t v)
{
	for (bitpos_t i = 0; i < bs->size; i++) {
		bitstream_write_bit(bs, v);
	}
}

void bitstream_seek(bitstream_t *bs, bitpos_t pos)
{
	bs->pos = pos;
}
bitpos_t bitstream_tell(bitstream_t *bs)
{
	return bs->pos;
}
void bitstream_rewind(bitstream_t *bs)
{
	bitstream_seek(bs, 0);
}

static bitpos_t read_iter_pos(bitstream_t *bs, bitpos_t i)
{
	bitpos_t pos = bs->iter ? bs->iter(bs, i, bs->opaque) : i;
	if (pos == bs->size) return BITPOS_END;
	return pos;
}

bit_t bitstream_is_end(bitstream_t *bs)
{
	bitpos_t pos = read_iter_pos(bs, bs->pos);
	if (pos == BITPOS_END) return 1;
	return 0;
}

static bit_t bitstream_read_bit_at(bitstream_t *bs, bitpos_t pos)
{
	if (pos >= bs->size) return 0;

#ifndef NO_CALLBACK
	if (bs->read_bit_at) {
		return bs->read_bit_at(bs, pos, bs->opaque_read);
	}
#endif

	return READ_BIT(bs->bits, pos);
}

static bit_t bitstream_write_bit_at(bitstream_t *bs, bitpos_t pos, bit_t bit)
{
	if (pos >= bs->size) return 0;

#ifndef NO_CALLBACK
	if (bs->write_bit_at) {
		bs->write_bit_at(bs, pos, bs->opaque_write, bit);
		return 1;
	}
#endif

	WRITE_BIT(bs->bits, pos, bit);
	return 1;
}

bit_t bitstream_peek_bit(bitstream_t *bs, bitpos_t *i)
{
	bitpos_t pos;
	bitpos_t idx = i ? *i : bs->pos;
	do {
		pos = read_iter_pos(bs, idx);
		if (pos == BITPOS_END) return 0;
		idx++;
		if (i) *i = idx;
	} while (pos == BITPOS_TRUNC);
	if (pos == BITPOS_BLANK) return 0;

	return bitstream_read_bit_at(bs, pos & BITPOS_MASK) ^ ((pos & BITPOS_TOGGLE) ? 1 : 0);
}

bit_t bitstream_read_bit(bitstream_t *bs)
{
	return bitstream_peek_bit(bs, &bs->pos);
}

uint_fast32_t bitstream_read_bits(bitstream_t *bs, uint_fast8_t num_bits)
{
	uint_fast32_t val = 0;

	assert(0 <= num_bits && num_bits <= 32);

	for (uint_fast8_t i = 0; i < num_bits; i++) {
		val = (val << 1) | bitstream_read_bit(bs);
	}
	return val;
}

uint_fast32_t bitstream_peek_bits(bitstream_t *bs, uint_fast8_t num_bits)
{
	uint_fast32_t val = 0;

	assert(0 <= num_bits && num_bits <= 32);

	bitpos_t iter_idx = bs->pos;
	for (uint_fast8_t i = 0; i < num_bits; i++) {
		val = (val << 1) | bitstream_peek_bit(bs, &iter_idx);
	}
	return val;
}

uint_fast8_t bitstream_skip_bits(bitstream_t *bs, uint_fast8_t num_bits)
{
	uint_fast8_t skipped = 0;
	for (uint_fast8_t i = 0; i < num_bits && !bitstream_is_end(bs); i++) {
		bitstream_read_bit(bs);
	}
	return skipped;
}

bit_t bitstream_write_bit(bitstream_t *bs, bit_t bit)
{
	bitpos_t pos;

	do {
		pos = read_iter_pos(bs, bs->pos);
		if (pos == BITPOS_END) return 0;
		bs->pos++;
	} while (pos == BITPOS_TRUNC);
	if (pos == BITPOS_BLANK) return 1;

	return bitstream_write_bit_at(bs, (pos & BITPOS_MASK), ((bit ? 1 : 0) ^ ((pos & BITPOS_TOGGLE) ? 1 : 0)));
}

bit_t bitstream_write_bits(bitstream_t *bs, uint_fast32_t value, uint_fast8_t num_bits)
{
	assert(0 <= num_bits && num_bits <= 32);

	for (uint_fast8_t i = 0; i < num_bits; i++) {
		if (!bitstream_write_bit(bs, (value & (1 << (num_bits - 1 - i))) ? 1 : 0)) {
			return 0;
		}
	}
	return 1;
}

bitpos_t bitstream_write_string(bitstream_t *bs, const char *fmt, ...)
{
#ifndef NO_PRINTF
	char buf[1024];
	bitpos_t i = 0;
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	while (buf[i] && !bitstream_is_end(bs) && i < sizeof(buf)) {
		bitstream_write_bits(bs, buf[i++], 8);
	}
	return i;
#else
	return 0;
#endif
}

bitpos_t bitstream_copy(bitstream_t *dst, bitstream_t *src, bitpos_t size, bitpos_t n)
{
	bitpos_t i;
	for (i = 0; !size || !n || i < size * n; i++) {
		if (bitstream_is_end(src) || bitstream_is_end(dst)) break;
		if (!bitstream_write_bit(dst, bitstream_read_bit(src))) break;
	}
	return i;
}

bitpos_t bitstream_write(bitstream_t *dst, const void *buffer, bitpos_t size, bitpos_t n)
{
	bitstream_t src = create_bitstream(buffer, size, bitstream_loop_iter, NULL);
	return bitstream_copy(dst, &src, size, n);
}

bitpos_t bitstream_read(bitstream_t *src, void *buffer, bitpos_t size, bitpos_t n)
{
	bitstream_t dst = create_bitstream(buffer, size, bitstream_loop_iter, NULL);
	return bitstream_copy(&dst, src, size, n);
}

void bitstream_dump(bitstream_t *src, bitpos_t len, FILE *out)
{
#ifndef NO_PRINTF
#ifndef HEXDUMP_PRINTF
#define HEXDUMP_PRINTF(...) fprintf(out, __VA_ARGS__)
#endif
	size_t start_addr = 0;
	const int fold_size = 16;
	const int flag_print_chars = 1;

	size_t remain = len ? len : (16 * 8);

	while ((!len || remain > 0) && !bitstream_is_end(src)) {
		uint8_t buf[16];
		size_t n = bitstream_read(src, buf, MIN(remain, 16 * 8), 1);
		if (len) remain -= n;

		HEXDUMP_PRINTF("%08zx: ", start_addr);
		start_addr = (start_addr / fold_size) * fold_size;
		for (size_t i = 0; i < fold_size; i++) {
			if (i % 8 == 0) HEXDUMP_PRINTF(" ");

			if (i < BYTE_SIZE(n)) {
				HEXDUMP_PRINTF("%02x ", buf[i]);
			} else {
				HEXDUMP_PRINTF("   ");
			}
		}
		if (flag_print_chars) {
			HEXDUMP_PRINTF(" |");
			for (size_t i = 0; i < fold_size; i++) {
				if (i < BYTE_SIZE(n)) {
					HEXDUMP_PRINTF("%c", buf[i] >= 0x20 && buf[i] < 0x7f ? buf[i] : '.');
				} else {
					HEXDUMP_PRINTF(" ");
				}
			}
			HEXDUMP_PRINTF("|");
		}
		HEXDUMP_PRINTF("\n");
		start_addr += fold_size;
	}
#undef HEXDUMP_PRINTF
#endif
}

bitpos_t bitstream_loop_iter(bitstream_t *bs, bitpos_t i, void *opaque)
{
	return i % bs->size;
}

bitpos_t bitstream_reverse_iter(bitstream_t *bs, bitpos_t i, void *opaque)
{
	if (i >= bs->size) return BITPOS_END;
	return bs->size - 1 - i;
}

void bitstream_on_write_bit(bitstream_t *bs, bitstream_write_bit_callback_t cb, void *opaque)
{
#ifndef NO_CALLBACK
	bs->write_bit_at = cb;
	bs->opaque_write = opaque;
#endif
}

void bitstream_on_read_bit(bitstream_t *bs, bitstream_read_bit_callback_t cb, void *opaque)
{
#ifndef NO_CALLBACK
	bs->read_bit_at = cb;
	bs->opaque_read = opaque;
#endif
}

bitpos_t bitstream_write_unicode_as_utf8(bitstream_t *bs, uint32_t code)
{
	if (code < 0x80) {
		bitstream_write_bits(bs, code, 8);
		return 8 * 1;
	} else if (code < 0x800) {
		bitstream_write_bits(bs, 0xc0 | (code >> 6), 8);
		bitstream_write_bits(bs, 0x80 | (code & 0x3f), 8);
		return 8 * 2;
	} else if (code < 0x10000) {
		bitstream_write_bits(bs, 0xe0 | (code >> 12), 8);
		bitstream_write_bits(bs, 0x80 | ((code >> 6) & 0x3f), 8);
		bitstream_write_bits(bs, 0x80 | (code & 0x3f), 8);
		return 8 * 3;
	} else if (code < 0x200000) {
		bitstream_write_bits(bs, 0xf0 | (code >> 18), 8);
		bitstream_write_bits(bs, 0x80 | ((code >> 12) & 0x3f), 8);
		bitstream_write_bits(bs, 0x80 | ((code >> 6) & 0x3f), 8);
		bitstream_write_bits(bs, 0x80 | (code & 0x3f), 8);
		return 8 * 4;
	}

	return 0;
}

int32_t bitstream_read_unicode_from_utf8(bitstream_t *bs, size_t len, size_t *consumed)
{
	if (len < 1) return -1;
	uint8_t ch1 = bitstream_read_bits(bs, 8);
	int32_t code = -1;

	if (consumed) *consumed = 1;

	if (ch1 < 0x80) {
		code = ch1;
		if (consumed) *consumed = 1;
	} else if (ch1 < 0xc0) {
		// invalid
		return -1;
	} else if (ch1 < 0xe0) {
		if (len < 2) return -1;
		uint8_t ch2 = bitstream_read_bits(bs, 8);
		if (0x80 <= ch2 && ch2 <= 0xbf) {
			code = (ch1 & 0x1f) << 6;
			code |= (ch2 & 0x3f);
		}
		if (consumed) *consumed = 2;
	} else if (ch1 < 0xf0) {
		if (len < 3) return -1;
		uint8_t ch2 = bitstream_read_bits(bs, 8);
		uint8_t ch3 = bitstream_read_bits(bs, 8);
		if (0x80 <= ch2 && ch2 <= 0xbf && 0x80 <= ch3 && ch3 <= 0xbf) {
			code = (ch1 & 0xf) << 12;
			code |= (ch2 & 0x3f) << 6;
			code |= (ch3 & 0x3f);
		}
		if (consumed) *consumed = 3;
	} else if (ch1 < 0xf8) {
		if (len < 4) return -1;
		uint8_t ch2 = bitstream_read_bits(bs, 8);
		uint8_t ch3 = bitstream_read_bits(bs, 8);
		uint8_t ch4 = bitstream_read_bits(bs, 8);
		if (0x80 <= ch2 && ch2 <= 0xbf && 0x80 <= ch3 && ch3 <= 0xbf && 0x80 <= ch4 && ch4 <= 0xbf) {
			code = (ch1 & 0x7) << 18;
			code |= (ch2 & 0x3f) << 12;
			code |= (ch3 & 0x3f) << 6;
			code |= (ch4 & 0x3f);
		}
		if (consumed) *consumed = 4;
	}
	return code;
}
