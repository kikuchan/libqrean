#ifndef __QR_QRPAYLOAD_H__
#define __QR_QRPAYLOAD_H__

#include "bitstream.h"
#include "qrdata.h"
#include "qrtypes.h"

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
} qrpayload_t;

void qrpayload_init(qrpayload_t *qrs, qr_version_t version, qr_errorlevel_t level);
void qrpayload_deinit(qrpayload_t *qrs);

qrpayload_t create_qrpayload(qr_version_t version, qr_errorlevel_t level);
qrpayload_t create_qrpayload_for_string(qr_version_t version, qr_errorlevel_t level, const char *src);
void qrpayload_destroy(qrpayload_t *qrs);

#ifndef NO_MALLOC
qrpayload_t *new_qrpayload(qr_version_t version, qr_errorlevel_t level);
qrpayload_t *new_qrpayload_for_string(qr_version_t version, qr_errorlevel_t level, const char *src);
void qrpayload_free(qrpayload_t *qrs);
#endif

bitstream_t qrpayload_get_bitstream(qrpayload_t *qrs);
bitstream_t qrpayload_get_bitstream_for_data(qrpayload_t *qrs);
bitstream_t qrpayload_get_bitstream_for_error(qrpayload_t *qrs);

void qrpayload_set_error_words(qrpayload_t *qrs);
int qrpayload_fix_errors(qrpayload_t *qrs);

size_t qrpayload_read_string(qrpayload_t *payload, char *buffer, size_t size);
bit_t qrpayload_write_string(qrpayload_t *payload, const char *src, size_t len, qrdata_writer_t writer);

#endif /* __QR_QRPAYLOAD_H__ */
