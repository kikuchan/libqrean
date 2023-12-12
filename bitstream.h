#include <stdio.h>
#include <stdint.h>

#ifndef __QR_BITSTREAM_H__
#define __QR_BITSTREAM_H__

typedef uint_fast8_t bit_t;
typedef uint_fast16_t bitpos_t;

static const bitpos_t bitpos_trunc = 0xffff;
static const bitpos_t bitpos_blank = 0xfffe;
static const bitpos_t bitpos_end = 0xfffd;

typedef struct _bitstream_t bitstream_t;
typedef bitpos_t (*bitstream_iterator_t)(bitstream_t *bs, bitpos_t pos, void *opaque);
typedef void (*bitstream_set_callback_t)(bitstream_t *bs, bitpos_t pos, void *opaque, bit_t v);
typedef bit_t (*bitstream_get_callback_t)(bitstream_t *bs, bitpos_t pos, void *opaque);

struct _bitstream_t {
	bitpos_t size;
	uint8_t *bits;

	bitpos_t pos;

	bitstream_iterator_t iter;
	void *opaque;

#ifndef NO_CALLBACK
	bitstream_set_callback_t set_bit_at;
	void *opaque_set;

	bitstream_get_callback_t get_bit_at;
	void *opaque_get;
#endif
};

void bitstream_init(bitstream_t *bs, void *src, bitpos_t len, bitstream_iterator_t iter, void *opaque);
void bitstream_deinit(bitstream_t *bs);

bitstream_t create_bitstream(const void *src, bitpos_t len, bitstream_iterator_t iter, void *opaque);
void bitstream_destroy(bitstream_t *bs);

void bitstream_fill(bitstream_t *bs, bit_t v);

bitpos_t bitstream_length(bitstream_t *bs);

void bitstream_dump(bitstream_t *bs);

bit_t bitstream_get_bit(bitstream_t *bs);
uint_fast32_t bitstream_get_bits(bitstream_t *bs, uint_fast8_t num_bits);

int bitstream_set_bit(bitstream_t *bs, bit_t bit);
int bitstream_set_bits(bitstream_t *bs, uint_fast32_t value, uint_fast8_t num_bits);

void bitstream_seek(bitstream_t *bs, bitpos_t pos);
void bitstream_rewind(bitstream_t *bs);
bitpos_t bitstream_tell(bitstream_t *bs);

bitpos_t bitstream_loop_iter(bitstream_t *bs, bitpos_t i, void *opaque);

bitpos_t bitstream_write(bitstream_t *dst, const void *buffer, bitpos_t size, bitpos_t n);
bitpos_t bitstream_read(bitstream_t *src, void *buffer, bitpos_t size, bitpos_t n);
bitpos_t bitstream_copy(bitstream_t *dst, bitstream_t *src, bitpos_t size, bitpos_t n);

bit_t bitstream_is_end(bitstream_t *bs);

void bitstream_on_set(bitstream_t *bs, bitstream_set_callback_t cb, void *opaque);
void bitstream_on_get(bitstream_t *bs, bitstream_get_callback_t cb, void *opaque);

#endif /* __QR_BITSTREAM_H__ */
