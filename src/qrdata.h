#ifndef __QR_QRDATA_H__
#define __QR_QRDATA_H__

#include "bitstream.h"
#include "qrtypes.h"

typedef struct {
	bitstream_t bs;
	qr_version_t version;
} qrdata_t;

typedef enum {
	QR_DATA_MODE_ECI = 0b0111,
	QR_DATA_MODE_NUMERIC = 0b0001,
	QR_DATA_MODE_ALNUM = 0b0010,
	QR_DATA_MODE_8BIT = 0b0100,
	QR_DATA_MODE_KANIJ = 0b1000,
	QR_DATA_MODE_FNC1_1 = 0b0101,
	QR_DATA_MODE_FNC1_2 = 0b1001,
	QR_DATA_MODE_STRUCTURED = 0b0011,
	QR_DATA_MODE_END = 0b0000,

	QR_DATA_MODE_AUTO = 0b11111111, // XXX:
} qr_data_mode_t;

qrdata_t create_qrdata_for(bitstream_t bs, qr_version_t version);
qrdata_t *new_qrdata_for(bitstream_t bs, qr_version_t version);
void qrdata_free(qrdata_t *data);

typedef size_t (*qrdata_writer_t)(qrdata_t *data, const char *src, size_t len);

size_t qrdata_write_8bit_string(qrdata_t *data, const char *src, size_t len);
size_t qrdata_write_numeric_string(qrdata_t *data, const char *src, size_t len);
size_t qrdata_write_alnum_string(qrdata_t *data, const char *src, size_t len);
size_t qrdata_write_string(qrdata_t *data, const char *src, size_t len);

bit_t qrdata_finalize(qrdata_t *data);

size_t qrdata_parse(qrdata_t *data, void *buffer, size_t size);

#endif /* __QR_QRDATA_H__ */
