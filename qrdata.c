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
	bitstream_put_bits(&data->bs, QR_DATA_MODE_8BIT, 4);
	if (data->qrs->version < 10) {
		bitstream_put_bits(&data->bs, len, 8);
	} else {
		bitstream_put_bits(&data->bs, len, 16);
	}
	for (bitpos_t i = 0; i < len; i++) {
		bitstream_put_bits(&data->bs, src[i], 8);
	}
	return 0;
}

int qrdata_push_8bit_string(qrdata_t *data, const char *src)
{
	return qrdata_push_8bit(data, src, strlen(src));
}

int qrdata_finalize(qrdata_t *data)
{
	if (!bitstream_put_bits(&data->bs, QR_DATA_MODE_END, 4)) return 0;

	bitstream_put_bits(&data->bs, 0, bitstream_tell(&data->bs) % 8);

	bitpos_t pos = bitstream_tell(&data->bs);

	int flip = 0;
	for (; pos < data->bs.size; pos += 8) {
		bitstream_put_bits(&data->bs, (flip ^= 1) ? 0xEC : 0x11, 8);
	}

	qrstream_set_error_words(data->qrs);

	return 1;
}

int qrdata_push_string(qrdata_t *data, const char *src)
{
	// TODO:
	return qrdata_push_8bit_string(data, src);
}

size_t qrdata_parse(qrdata_t *data, void *buffer, size_t size)
{
	bitstream_t *r = &data->bs;
	bitstream_t bs = create_bitstream(buffer, size * 8, NULL, NULL);
	bitstream_t *w = &bs;
	bitpos_t len;

	while (!bitstream_is_end(r)) {
		uint8_t ch = bitstream_get_bits(r, 4);
		switch (ch) {
		case QR_DATA_MODE_END:
			goto end;

		case QR_DATA_MODE_8BIT:
			len = bitstream_get_bits(r, data->qrs->version > 10 ? 16 : 8); 
			while (len-- > 0) {
				bitstream_put_bits(w, bitstream_get_bits(r, 8), 8);
			}
			break;

		// TODO
		}
	}

end:;
	return bitstream_tell(w) / 8;
}
