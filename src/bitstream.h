#ifndef __QR_BITSTREAM_H__
#define __QR_BITSTREAM_H__

#include <stdint.h>
#include <stdio.h>

typedef uint_fast8_t bit_t;
typedef uint_fast32_t bitpos_t;

#define BITPOS_TRUNC  0xffffffff
#define BITPOS_BLANK  0xfffffffe
#define BITPOS_END    0xfffffffd
#define BITPOS_MASK   0x7fffffff
#define BITPOS_TOGGLE 0x80000000

typedef struct _bitstream_t bitstream_t;
typedef bitpos_t (*bitstream_iterator_t)(bitstream_t *bs, bitpos_t pos, void *opaque);
typedef void (*bitstream_write_bit_callback_t)(bitstream_t *bs, bitpos_t pos, void *opaque, bit_t v);
typedef bit_t (*bitstream_read_bit_callback_t)(bitstream_t *bs, bitpos_t pos, void *opaque);

struct _bitstream_t {
	bitpos_t size;
	uint8_t *bits;

	bitpos_t pos;

	bitstream_iterator_t iter;
	void *opaque;

#ifndef NO_CALLBACK
	bitstream_write_bit_callback_t write_bit_at;
	void *opaque_write;

	bitstream_read_bit_callback_t read_bit_at;
	void *opaque_read;
#endif
};

void bitstream_init(bitstream_t *bs, void *src, bitpos_t len, bitstream_iterator_t iter, void *opaque);

bitstream_t create_bitstream(const void *src, bitpos_t len, bitstream_iterator_t iter, void *opaque);

void bitstream_fill(bitstream_t *bs, bit_t v);

bitpos_t bitstream_length(bitstream_t *bs);

void bitstream_dump(bitstream_t *bs, bitpos_t len, FILE *out);

bit_t bitstream_read_bit(bitstream_t *bs);
bit_t bitstream_peek_bit(bitstream_t *bs, bitpos_t *pos);
uint_fast32_t bitstream_read_bits(bitstream_t *bs, uint_fast8_t num_bits);
uint_fast32_t bitstream_peek_bits(bitstream_t *bs, uint_fast8_t num_bits);

uint_fast8_t bitstream_skip_bits(bitstream_t *bs, uint_fast8_t num_bits);

bit_t bitstream_write_bit(bitstream_t *bs, bit_t bit);
bit_t bitstream_write_bits(bitstream_t *bs, uint_fast32_t value, uint_fast8_t num_bits);
bitpos_t bitstream_write_string(bitstream_t *bs, const char *fmt, ...);
bitpos_t bitstream_write_unicode_as_utf8(bitstream_t *bs, uint32_t code);
int32_t bitstream_read_unicode_from_utf8(bitstream_t *bs, size_t len, size_t *consumed);

void bitstream_seek(bitstream_t *bs, bitpos_t pos);
void bitstream_rewind(bitstream_t *bs);
bitpos_t bitstream_tell(bitstream_t *bs);

bitpos_t bitstream_loop_iter(bitstream_t *bs, bitpos_t i, void *opaque);
bitpos_t bitstream_reverse_iter(bitstream_t *bs, bitpos_t i, void *opaque);

bitpos_t bitstream_write(bitstream_t *dst, const void *buffer, bitpos_t size, bitpos_t n);
bitpos_t bitstream_read(bitstream_t *src, void *buffer, bitpos_t size, bitpos_t n);
bitpos_t bitstream_copy(bitstream_t *dst, bitstream_t *src, bitpos_t size, bitpos_t n);

bit_t bitstream_is_end(bitstream_t *bs);

void bitstream_on_write_bit(bitstream_t *bs, bitstream_write_bit_callback_t cb, void *opaque);
void bitstream_on_read_bit(bitstream_t *bs, bitstream_read_bit_callback_t cb, void *opaque);

#endif /* __QR_BITSTREAM_H__ */
