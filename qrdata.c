#include <string.h>
#include "qrdata.h"
#include "bitstream.h"

qrdata_t create_qrdata_for(qrstream_t *qrs) {
	qrdata_t data = {
		.bs = qrstream_get_bitstream_for_data(qrs),
		.qrs = qrs,
	};
	return data;
}

int qrdata_push_8bit(qrdata_t *data, const char *src, size_t len)
{
	bitstream_set_bits(&data->bs, QR_DATA_MODE_8BIT, 4);
	if (data->qrs->version < 10) {
		bitstream_set_bits(&data->bs, len, 8);
	} else {
		bitstream_set_bits(&data->bs, len, 16);
	}
	for (bitpos_t i = 0; i < len; i++) {
		bitstream_set_bits(&data->bs, src[i], 8);
	}
	return 0;
}

int qrdata_push_8bit_string(qrdata_t *data, const char *src)
{
	return qrdata_push_8bit(data, src, strlen(src));
}

int qrdata_finalize(qrdata_t *data)
{
	if (!bitstream_set_bits(&data->bs, QR_DATA_MODE_END, 4)) return 0;

	bitstream_set_bits(&data->bs, 0, bitstream_tell(&data->bs) % 8);

	bitpos_t pos = bitstream_tell(&data->bs);

	int flip = 0;
	for (; pos < data->bs.size; pos += 8) {
		bitstream_set_bits(&data->bs, (flip ^= 1) ? 0xEC : 0x11, 8);
	}

	qrstream_set_error_words(data->qrs);

	return 1;
}

int qrdata_push_string(qrdata_t *data, const char *src)
{
	// TODO:
	return qrdata_push_8bit_string(data, src);
}
