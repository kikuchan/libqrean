#include "qrstream.h"

typedef struct {
	bitstream_t bs;
	qrstream_t *qrs;
} qrdata_t;

typedef enum {
	QR_DATA_MODE_END = 0b0000,
	QR_DATA_MODE_NUMERIC = 0b0001,
	QR_DATA_MODE_ALNUM = 0b0010,
	QR_DATA_MODE_8BIT = 0b0100,
	QR_DATA_MODE_KANIJ = 0b1000,
} qr_data_mode_t;

qrdata_t create_qrdata_for(qrstream_t *qrs);
qrdata_t *new_qrdata_for(qrstream_t *qrs);
void qrdata_free(qrdata_t *data);

size_t qrdata_write_8bit_string(qrdata_t *data, const char *src, size_t len);
size_t qrdata_write_numeric_string(qrdata_t *data, const char *src, size_t len);
size_t qrdata_write_alnum_string(qrdata_t *data, const char *src, size_t len);
size_t qrdata_write_string(qrdata_t *data, const char *src, size_t len);
bit_t qrdata_finalize(qrdata_t *data);

size_t qrdata_parse(qrdata_t *data, void *buffer, size_t size);
