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

#define VERDEPLEN(version, a, b, c) ((version) < QR_VERSION_10 ? (a) : (version) < QR_VERSION_27 ? (b) : (c))

#define LENGTH_BIT_SIZE_FOR_8BIT(version)    VERDEPLEN(version, 8, 16, 16)
#define LENGTH_BIT_SIZE_FOR_NUMERIC(version) VERDEPLEN(version, 10, 12, 14)
#define LENGTH_BIT_SIZE_FOR_ALNUM(version)   VERDEPLEN(version, 9, 11, 13)

#define BITSIZE_FOR_NUMERIC(len) ((len) >= 3 ? 10 : (len) == 2 ? 7 : 4)
#define BITSIZE_FOR_ALNUM(len)   (11)

static const char alnum_cmp[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:";
static const char alnum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:";

#define IS_NUMERIC(ch) ('0' <= (ch) && (ch) <= '9')
static size_t measure_numeric(const char *src) {
	int len;
	for (len = 0; src[len] && IS_NUMERIC(src[len]); len++) /* COUNT */
		;
	return len;
}

static size_t measure_alnum(const char *src) {
	return strspn(src, alnum);
}

#define LEN_CMP_8BIT(ch)  (strcspn((ch), alnum))
#define LEN_CMP_ALNUM(ch) (strspn((ch), alnum_cmp))
#define LEN_NUMERIC(ch)   (measure_numeric(ch))
#define LEN_ALNUM(ch)     (measure_alnum(ch))

size_t qrdata_write_8bit_string(qrdata_t *data, const char *src, size_t len) {
	if (len == 0) return 0;

	bitstream_write_bits(&data->bs, QR_DATA_MODE_8BIT, 4);
	bitstream_write_bits(&data->bs, len, LENGTH_BIT_SIZE_FOR_8BIT(data->qrs->version));

	size_t i;
	for (i = 0; i < len && !bitstream_is_end(&data->bs); i++) {
		bitstream_write_bits(&data->bs, src[i], 8);
	}
	return i;
}

size_t qrdata_write_numeric_string(qrdata_t *data, const char *src, size_t len) {
	if (measure_numeric(src) < len) {
		return 0;
	}

	bitstream_write_bits(&data->bs, QR_DATA_MODE_NUMERIC, 4);
	bitstream_write_bits(&data->bs, len, LENGTH_BIT_SIZE_FOR_NUMERIC(data->qrs->version));

	size_t i = 0;
	for (i = 0; i < len && !bitstream_is_end(&data->bs);) {
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

size_t qrdata_write_alnum_string(qrdata_t *data, const char *src, size_t len) {
	if (measure_alnum(src) < len) return 0;

	bitstream_write_bits(&data->bs, QR_DATA_MODE_ALNUM, 4);
	bitstream_write_bits(&data->bs, len, LENGTH_BIT_SIZE_FOR_ALNUM(data->qrs->version));

	size_t i = 0;
	for (i = 0; i < len && !bitstream_is_end(&data->bs);) {
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

static size_t qrdata_flush(qrdata_t *data, qr_data_mode_t mode, const char *src, size_t len) {
	switch (mode) {
	case QR_DATA_MODE_NUMERIC:
		return qrdata_write_numeric_string(data, src, len);

	case QR_DATA_MODE_ALNUM:
		return qrdata_write_alnum_string(data, src, len);

	case QR_DATA_MODE_8BIT:
		return qrdata_write_8bit_string(data, src, len);

	default:
		return 0;
	}
}

size_t qrdata_write_string(qrdata_t *data, const char *src, size_t len) {
	uint8_t v = data->qrs->version;

	size_t i, l;
	qr_data_mode_t mode;

	// initial mode
	if (LEN_CMP_8BIT(src) > 0) {
		mode = QR_DATA_MODE_8BIT;
	} else if ((l = LEN_CMP_ALNUM(src)) > 0) {
		if (l < VERDEPLEN(v, 6, 7, 8) && l < len) {
			mode = QR_DATA_MODE_8BIT;
		} else {
			mode = QR_DATA_MODE_ALNUM;
		}
	} else {
		l = LEN_NUMERIC(src);
		if (l < VERDEPLEN(v, 4, 4, 5) && LEN_CMP_8BIT(src + l) > 0) {
			mode = QR_DATA_MODE_8BIT;
		} else if (l < VERDEPLEN(v, 7, 8, 9) && LEN_CMP_ALNUM(src + l) > 0) {
			mode = QR_DATA_MODE_ALNUM;
		} else {
			mode = QR_DATA_MODE_NUMERIC;
		}
	}

	size_t last_i = 0;
	qr_data_mode_t last_mode = mode;

	for (i = 0; i < len; i++) {
		if (mode == QR_DATA_MODE_8BIT) {
			if (LEN_NUMERIC(src + i) >= VERDEPLEN(v, 6, 8, 9)) {
				mode = QR_DATA_MODE_NUMERIC;
			} else if (LEN_ALNUM(src + i) >= VERDEPLEN(v, 11, 15, 16)) {
				mode = QR_DATA_MODE_ALNUM;
			}
		} else if (mode == QR_DATA_MODE_ALNUM) {
			if (LEN_CMP_8BIT(src + i) > 0) {
				mode = QR_DATA_MODE_8BIT;
			} else if (LEN_NUMERIC(src + i) >= VERDEPLEN(v, 13, 15, 17)) {
				mode = QR_DATA_MODE_NUMERIC;
			}
		} else if (mode == QR_DATA_MODE_NUMERIC) {
			if (LEN_CMP_8BIT(src + i) > 0) {
				mode = QR_DATA_MODE_8BIT;
			} else if (LEN_CMP_ALNUM(src + i) > 0) {
				mode = QR_DATA_MODE_ALNUM;
			}
		} else {
			return 0;
		}

		if (mode != last_mode) {
			size_t l = qrdata_flush(data, last_mode, src + last_i, i - last_i);
			if (last_i + l < i) return last_i + l;
			last_mode = mode;
			last_i = i;
		}
	}

	size_t r = last_i + qrdata_flush(data, last_mode, src + last_i, len - last_i);
	return r;
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
