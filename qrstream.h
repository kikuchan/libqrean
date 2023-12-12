#ifndef __QR_QRSTREAM_H__
#define __QR_QRSTREAM_H__

#include "types.h"
#include "bitstream.h"

#define RSBLOCK_BUFFER_SIZE (3706)

typedef struct {
	qr_version_t version;
	qr_errorlevel_t level;

	bitpos_t data_words;
	bitpos_t error_words;
	bitpos_t total_words;

	bitpos_t total_bits;

	bitpos_t small_blocks;
	bitpos_t large_blocks;
	bitpos_t total_blocks;

	bitpos_t data_words_in_small_block;
	bitpos_t data_words_in_large_block;

	bitpos_t error_words_in_block;

	bitpos_t total_words_in_small_block;
	bitpos_t total_words_in_large_block;

#if defined(USE_MALLOC_BUFFER) && defined(NO_MALLOC)
#error "Specify both of USE_MALLOC_BUFFER and NO_MALLOC doesn't make sense"
#endif
#if defined(USE_MALLOC_BUFFER) && !defined(NO_MALLOC)
	uint8_t *buffer;
#else
	uint8_t buffer[RSBLOCK_BUFFER_SIZE];
#endif
} qrstream_t;

void qrstream_init(qrstream_t *qrs, qr_version_t version, qr_errorlevel_t level);
void qrstream_deinit(qrstream_t *qrs);

qrstream_t create_qrstream(qr_version_t version, qr_errorlevel_t level);
qrstream_t create_qrstream_for_string(qr_version_t version, qr_errorlevel_t level, const char *src);
void qrstream_destroy(qrstream_t *qrs);

#ifndef NO_MALLOC
qrstream_t* new_qrstream(qr_version_t version, qr_errorlevel_t level);
qrstream_t *new_qrstream_for_string(qr_version_t version, qr_errorlevel_t level, const char *src);
void qrstream_free(qrstream_t *qrs);
#endif


bitstream_t qrstream_get_bitstream(qrstream_t *qrs);
bitstream_t qrstream_get_bitstream_for_data(qrstream_t *qrs);
bitstream_t qrstream_get_bitstream_for_error(qrstream_t *qrs);

void qrstream_set_error_words(qrstream_t *qrs);
int qrstream_fix_errors(qrstream_t *qrs);

int qrstream_set_string(qrstream_t *qrs, const char *src);

#endif /* __QR_QRSTREAM_H__ */
