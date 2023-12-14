#include "qrdata.h"
#include "bitstream.h"
#include <string.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

qrdata_t create_qrdata_for(qrstream_t *qrs) {
	qrdata_t data = {
		.bs = qrstream_read_bitstream_for_data(qrs),
		.qrs = qrs,
	};
	return data;
}

#define LENGTH_BIT_SIZE_FOR_8BIT(version) ((version) > 10 ? 16 : 8)
#define LENGTH_BIT_SIZE_FOR_NUMERIC(version) ((version) < QR_VERSION_10 ? 10 : (version) < QR_VERSION_27 ? 12 : 14)
#define LENGTH_BIT_SIZE_FOR_ALNUM(version) ((version) < QR_VERSION_10 ? 9 : (version) < QR_VERSION_27 ? 11 : 13)
#define BITSIZE_FOR_NUMERIC(len) ((len) >= 3 ? 10 : (len) == 2 ? 7 : 4)
#define BITSIZE_FOR_ALNUM(len) (11)

size_t qrdata_push_8bit(qrdata_t *data, const char *src, size_t len) {
	if (len == 0) return 0;

	bitstream_write_bits(&data->bs, QR_DATA_MODE_8BIT, 4);
	bitstream_write_bits(&data->bs, len, LENGTH_BIT_SIZE_FOR_8BIT(data->qrs->version));

	size_t i;
	for (i = 0; i < len && !bitstream_is_end(&data->bs); i++) {
		bitstream_write_bits(&data->bs, src[i], 8);
	}
	return i;
}

size_t qrdata_measure_8bit_string(qrdata_t *data, const char *src) {
	return strlen(src);
}
size_t qrdata_push_8bit_string(qrdata_t *data, const char *src) {
	size_t len = qrdata_measure_8bit_string(data, src);
	return qrdata_push_8bit(data, src, len);
}

#define IS_NUMERIC(ch) ('0' <= (ch) && (ch) <= '9')
size_t qrdata_measure_numeric(qrdata_t *data, const char *src) {
	int len;
	for (len = 0; src[len] && IS_NUMERIC(src[len]); len++) /* COUNT */;
	return len;
}
size_t qrdata_push_numeric(qrdata_t *data, const char *src) {
	int len = qrdata_measure_numeric(data, src);
	if (len == 0) return 0;

	bitstream_write_bits(&data->bs, QR_DATA_MODE_NUMERIC, 4);
	bitstream_write_bits(&data->bs, len, LENGTH_BIT_SIZE_FOR_NUMERIC(data->qrs->version));

	size_t i = 0;
	for (i = 0; i < len && !bitstream_is_end(&data->bs); ) {
		size_t remain = MIN(len - i, 3);
		uint_fast16_t n = 0;

		for (uint_fast8_t x = 0; x < remain; x++) {
			uint_fast8_t v = src[i++] - '0';
			n = n * 10 + v;
		}

		bitstream_write_bits(&data->bs, n, BITSIZE_FOR_NUMERIC(remain));
	}

	return i;
}

static const char alnum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:";
size_t qrdata_measure_alnum(qrdata_t *data, const char *src) {
	return strspn(src, alnum);
}
size_t qrdata_push_alnum(qrdata_t *data, const char *src) {
	int len = qrdata_measure_alnum(data, src);
	if (len == 0) return 0;

	bitstream_write_bits(&data->bs, QR_DATA_MODE_ALNUM, 4);
	bitstream_write_bits(&data->bs, len, LENGTH_BIT_SIZE_FOR_ALNUM(data->qrs->version));

	size_t i = 0;
	for (i = 0; i < len && !bitstream_is_end(&data->bs); ) {
		size_t remain = MIN(len - i, 2);
		uint_fast16_t n = 0;

		for (uint_fast8_t x = 0; x < remain; x++) {
			uint_fast8_t v = strchr(alnum, src[i++]) - alnum;
			n = n * 45 + v;
		}

		bitstream_write_bits(&data->bs, n, BITSIZE_FOR_ALNUM(remain));
	}

	return i;
}

bit_t qrdata_finalize(qrdata_t *data) {
	if (!bitstream_write_bits(&data->bs, QR_DATA_MODE_END, 4)) return 0;

	bitstream_write_bits(&data->bs, 0, bitstream_tell(&data->bs) % 8);

	size_t pos = bitstream_tell(&data->bs);

	int flip = 0;
	for (; pos < data->bs.size; pos += 8) {
		bitstream_write_bits(&data->bs, (flip ^= 1) ? 0xEC : 0x11, 8);
	}

	qrstream_set_error_words(data->qrs);

	return 1;
}

size_t qrdata_push_string(qrdata_t *data, const char *src) {
	size_t len = strlen(src);

	size_t i;
	// TODO: use better method for efficiency
	for (i = 0; i < len; ) {
		size_t l;

		if ((l = qrdata_push_numeric(data, src + i)) > 0) { i += l; continue; }
		if ((l = qrdata_push_alnum(data, src + i)) > 0) { i += l; continue; }
		if ((l = qrdata_push_8bit_string(data, src + i)) > 0) { i += l; continue; }

		// overflow
		return i;
	}

	// done
	return i;
}


size_t qrdata_parse(qrdata_t *data, void *buffer, size_t size) {
	bitstream_t *r = &data->bs;
	bitstream_t bs = create_bitstream(buffer, size * 8, NULL, NULL);
	bitstream_t *w = &bs;
	size_t len;

	while (!bitstream_is_end(r)) {
		uint8_t ch = bitstream_read_bits(r, 4);
		switch (ch) {
		case QR_DATA_MODE_END:
#ifdef DEBUG_QRDATA
			bitstream_write_string(w, "[END]");
#endif
			goto end;

		case QR_DATA_MODE_NUMERIC:
#ifdef DEBUG_QRDATA
			bitstream_write_string(w, "[NUM]");
#endif
			len = bitstream_read_bits(r, LENGTH_BIT_SIZE_FOR_NUMERIC(data->qrs->version));
			while (len > 0) {
				uint16_t v = bitstream_read_bits(r, len >= 3 ? 10 : len == 2 ? 7 : 4);

				if (len >= 3) {
					bitstream_write_bits(w, '0' + v / 100 % 10, 8);
					len--;
				}
				if (len >= 2) {
					bitstream_write_bits(w, '0' + v / 10 % 10, 8);
					len--;
				}
				bitstream_write_bits(w, '0' + v % 10, 8);
				len--;
			}
			break;

		case QR_DATA_MODE_ALNUM:
#ifdef DEBUG_QRDATA
			bitstream_write_string(w, "[ALNUM]");
#endif
			len = bitstream_read_bits(r, LENGTH_BIT_SIZE_FOR_ALNUM(data->qrs->version));
			while (len > 0) {
				uint16_t v = bitstream_read_bits(r, 11);
				if (len >= 2) {
					bitstream_write_bits(w, alnum[v / 45 % 45], 8);
					if (--len == 0) break;
				}
				bitstream_write_bits(w, alnum[v % 45], 8);
				if (--len == 0) break;
			}
			break;

		case QR_DATA_MODE_8BIT:
#ifdef DEBUG_QRDATA
			bitstream_write_string(w, "[8BIT]");
#endif
			len = bitstream_read_bits(r, LENGTH_BIT_SIZE_FOR_8BIT(data->qrs->version));
			while (len-- > 0) {
				bitstream_write_bits(w, bitstream_read_bits(r, 8), 8);
			}
			break;

		default:
			// unknown
#ifdef DEBUG_QRDATA
			bitstream_write_string(w, "[?]");
#endif
			goto end;
		}
	}

end:;
	return bitstream_tell(w) / 8;
}
