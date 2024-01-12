#ifndef __QR_QRDATA_H__
#define __QR_QRDATA_H__

#include "bitstream.h"
#include "qrspec.h"

typedef enum {
	QR_ECI_CODE_LATIN1 = 3,
	QR_ECI_CODE_SJIS = 20,
	QR_ECI_CODE_UTF8 = 26,
	QR_ECI_CODE_BINARY = 899,
} qr_eci_code_t;

typedef struct {
	bitstream_t bs;
	qr_version_t version;
	qr_eci_code_t eci_code;
	qr_eci_code_t eci_last;
} qrdata_t;

typedef enum {
	QR_DATA_MODE_ECI = 0b0111,
	QR_DATA_MODE_NUMERIC = 0b0001,
	QR_DATA_MODE_ALNUM = 0b0010,
	QR_DATA_MODE_8BIT = 0b0100,
	QR_DATA_MODE_KANJI = 0b1000,
	QR_DATA_MODE_FNC1_1 = 0b0101,
	QR_DATA_MODE_FNC1_2 = 0b1001,
	QR_DATA_MODE_STRUCTURED = 0b0011,
	QR_DATA_MODE_END = 0b0000,

	QR_DATA_MODE_AUTO = 0b11111111, // XXX:
} qr_data_mode_t;

typedef enum {
	QR_DATA_LETTER_TYPE_RAW = 0,
	QR_DATA_LETTER_TYPE_UNICODE,
	QR_DATA_LETTER_TYPE_ECI_CHANGE,
	QR_DATA_LETTER_TYPE_END,
} qr_data_letter_type_t;

qrdata_t create_qrdata_for(bitstream_t bs, qr_version_t version, qr_eci_code_t code);
qrdata_t *new_qrdata_for(bitstream_t bs, qr_version_t version);
void qrdata_free(qrdata_t *data);

typedef size_t (*qrdata_writer_t)(qrdata_t *data, const char *src, size_t len);

size_t qrdata_write_8bit_string(qrdata_t *data, const char *src, size_t len);
size_t qrdata_write_kanji_string(qrdata_t *data, const char *src, size_t len);
size_t qrdata_write_numeric_string(qrdata_t *data, const char *src, size_t len);
size_t qrdata_write_alnum_string(qrdata_t *data, const char *src, size_t len);
size_t qrdata_write_string(qrdata_t *data, const char *src, size_t len);

bitpos_t qrdata_write_eci_code(qrdata_t *data, qr_eci_code_t eci);

bit_t qrdata_finalize(qrdata_t *data);

typedef size_t (*qrdata_parse_callback_t)(qr_data_letter_type_t type, const uint32_t letter, void *opaque);
size_t qrdata_parse(qrdata_t *data, qrdata_parse_callback_t on_letter_cb, void *opaque);

#endif /* __QR_QRDATA_H__ */
