#include "qrdata.h"
#include "bitstream.h"
#include "debug.h"
#include "qrspec.h"
#include "utils.h"
#include <string.h>

qrdata_t create_qrdata_for(bitstream_t bs, qr_version_t version)
{
	qrdata_t data = {
		.bs = bs,
		.version = version,
	};
	return data;
}

#define VERDEPNUM(version, a, b, c) ((version) < QR_VERSION_10 ? (a) : (version) < QR_VERSION_27 ? (b) : (c))

#define LENGTH_BIT_SIZE_TBL(version, n) qrspec_get_data_bitlength_for(version, n)

#define LENGTH_BIT_SIZE_FOR_NUMERIC(version) LENGTH_BIT_SIZE_TBL(version, 0)
#define LENGTH_BIT_SIZE_FOR_ALNUM(version)   LENGTH_BIT_SIZE_TBL(version, 1)
#define LENGTH_BIT_SIZE_FOR_8BIT(version)    LENGTH_BIT_SIZE_TBL(version, 2)
#define LENGTH_BIT_SIZE_FOR_KANJI(version)   LENGTH_BIT_SIZE_TBL(version, 3)

#define BITSIZE_FOR_NUMERIC(len) ((len) >= 3 ? 10 : (len) == 2 ? 7 : 4)
#define BITSIZE_FOR_ALNUM(len)   ((len % 2) ? 6 : 11)

static const char alnum_cmp[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:";
static const char alnum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:";

#define IS_NUMERIC(ch) ('0' <= (ch) && (ch) <= '9')
static size_t measure_numeric(const char *src)
{
	int len;
	for (len = 0; src[len] && IS_NUMERIC(src[len]); len++) /* COUNT */
		;
	return len;
}

static size_t measure_alnum(const char *src)
{
	return strspn(src, alnum);
}

#define LEN_CMP_8BIT(ch)  (strcspn((ch), alnum))
#define LEN_CMP_ALNUM(ch) (strspn((ch), alnum_cmp))
#define LEN_NUMERIC(ch)   (measure_numeric(ch))
#define LEN_ALNUM(ch)     (measure_alnum(ch))

#define RMQR_DATA_MODE_END     (0)
#define RMQR_DATA_MODE_NUMERIC (1)
#define RMQR_DATA_MODE_ALNUM   (2)
#define RMQR_DATA_MODE_8BIT    (3)
#define RMQR_DATA_MODE_KANJI   (4)

size_t qrdata_write_numeric_string(qrdata_t *data, const char *src, size_t len)
{
	if (measure_numeric(src) < len) {
		return 0;
	}

	if (IS_MQR(data->version)) {
		bitstream_write_bits(&data->bs, 0, data->version - QR_VERSION_M1);
	} else if (IS_RMQR(data->version)) {
		bitstream_write_bits(&data->bs, RMQR_DATA_MODE_NUMERIC, 3);
	} else if (IS_QR(data->version)) {
		bitstream_write_bits(&data->bs, QR_DATA_MODE_NUMERIC, 4);
	}
	bitstream_write_bits(&data->bs, len, LENGTH_BIT_SIZE_FOR_NUMERIC(data->version));

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

size_t qrdata_write_alnum_string(qrdata_t *data, const char *src, size_t len)
{
	if (measure_alnum(src) < len) return 0;

	if (IS_MQR(data->version)) {
		if (data->version == QR_VERSION_M1) return 0;
		bitstream_write_bits(&data->bs, 1, data->version - QR_VERSION_M1);
	} else if (IS_RMQR(data->version)) {
		bitstream_write_bits(&data->bs, RMQR_DATA_MODE_ALNUM, 3);
	} else {
		bitstream_write_bits(&data->bs, QR_DATA_MODE_ALNUM, 4);
	}
	bitstream_write_bits(&data->bs, len, LENGTH_BIT_SIZE_FOR_ALNUM(data->version));

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

size_t qrdata_write_8bit_string(qrdata_t *data, const char *src, size_t len)
{
	if (len == 0) return 0;

	if (IS_MQR(data->version)) {
		if (data->version <= QR_VERSION_M2) return 0;
		bitstream_write_bits(&data->bs, 2, data->version - QR_VERSION_M1);
	} else if (IS_RMQR(data->version)) {
		bitstream_write_bits(&data->bs, RMQR_DATA_MODE_8BIT, 3);
	} else {
		bitstream_write_bits(&data->bs, QR_DATA_MODE_8BIT, 4);
	}

	bitstream_write_bits(&data->bs, len, LENGTH_BIT_SIZE_FOR_8BIT(data->version));

	size_t i;
	for (i = 0; i < len && !bitstream_is_end(&data->bs); i++) {
		bitstream_write_bits(&data->bs, src[i], 8);
	}
	return i;
}

bit_t qrdata_finalize(qrdata_t *data)
{
	if (IS_MQR(data->version)) {
		if (!bitstream_write_bits(&data->bs, 0, 3 + 2 * (data->version - QR_VERSION_M1))) return 0;
	} else if (IS_RMQR(data->version)) {
		if (!bitstream_write_bits(&data->bs, RMQR_DATA_MODE_END, 3)) return 0;
	} else {
		if (!bitstream_write_bits(&data->bs, QR_DATA_MODE_END, 4)) return 0;
	}
	if (bitstream_tell(&data->bs) % 8) bitstream_write_bits(&data->bs, 0, 8 - (bitstream_tell(&data->bs) % 8));

	size_t pos = bitstream_tell(&data->bs);

	int flip = 0;
	for (; pos < data->bs.size; pos += 8) {
		bitstream_write_bits(&data->bs, (flip ^= 1) ? 0xEC : 0x11, 8);
	}

	return 1;
}

static size_t qrdata_flush(qrdata_t *data, qr_data_mode_t mode, const char *src, size_t len)
{
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

size_t qrdata_write_string(qrdata_t *data, const char *src, size_t len)
{
	uint8_t v = data->version;

	size_t i, l;
	qr_data_mode_t mode;

	// initial mode
	if (LEN_CMP_8BIT(src) > 0) {
		mode = QR_DATA_MODE_8BIT;
	} else if ((l = LEN_CMP_ALNUM(src)) > 0) {
		if (l < VERDEPNUM(v, 6, 7, 8) && l < len) {
			mode = QR_DATA_MODE_8BIT;
		} else {
			mode = QR_DATA_MODE_ALNUM;
		}
	} else {
		l = LEN_NUMERIC(src);
		if (l < VERDEPNUM(v, 4, 4, 5) && LEN_CMP_8BIT(src + l) > 0) {
			mode = QR_DATA_MODE_8BIT;
		} else if (l < VERDEPNUM(v, 7, 8, 9) && LEN_CMP_ALNUM(src + l) > 0) {
			mode = QR_DATA_MODE_ALNUM;
		} else {
			mode = QR_DATA_MODE_NUMERIC;
		}
	}

	size_t last_i = 0;
	qr_data_mode_t last_mode = mode;

	for (i = 0; i < len; i++) {
		if (mode == QR_DATA_MODE_8BIT) {
			if (LEN_NUMERIC(src + i) >= VERDEPNUM(v, 6, 8, 9)) {
				mode = QR_DATA_MODE_NUMERIC;
			} else if (LEN_ALNUM(src + i) >= VERDEPNUM(v, 11, 15, 16)) {
				mode = QR_DATA_MODE_ALNUM;
			}
		} else if (mode == QR_DATA_MODE_ALNUM) {
			if (LEN_CMP_8BIT(src + i) > 0) {
				mode = QR_DATA_MODE_8BIT;
			} else if (LEN_NUMERIC(src + i) >= VERDEPNUM(v, 13, 15, 17)) {
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

size_t qrdata_parse(qrdata_t *data, void (*on_letter_cb)(qr_data_mode_t mode, const uint32_t letter, void *opaque), void *opaque)
{
	bitstream_t *r = &data->bs;
	size_t len;
	size_t wrote = 0;

	qr_data_mode_t mode = QR_DATA_MODE_NUMERIC; // for mQR
	while (!bitstream_is_end(r)) {
		if (IS_MQR(data->version)) {
			if (data->version != QR_VERSION_M1) {
				switch (bitstream_read_bits(r, data->version - QR_VERSION_M1)) {
				case 0:
					mode = QR_DATA_MODE_NUMERIC;
					break;
				case 1:
					mode = QR_DATA_MODE_ALNUM;
					break;
				case 2:
					mode = QR_DATA_MODE_8BIT;
					break;
				case 3:
					mode = QR_DATA_MODE_KANJI;
					break;
				}
			}
		} else if (IS_QR(data->version)) {
			mode = (qr_data_mode_t)bitstream_read_bits(r, 4);
		} else if (IS_RMQR(data->version)) {
			uint8_t m = bitstream_read_bits(r, 3);
			switch (m) {
			case 0:
				mode = QR_DATA_MODE_END;
				break;
			case 1:
				mode = QR_DATA_MODE_NUMERIC;
				break;
			case 2:
				mode = QR_DATA_MODE_ALNUM;
				break;
			case 3:
				mode = QR_DATA_MODE_8BIT;
				break;
			case 4:
				mode = QR_DATA_MODE_KANJI;
				break;
			default:
				qrean_debug_printf("Unknown mode: %03b\n", m);
				break;
			}
		}
		switch (mode) {
		case QR_DATA_MODE_END:
		mode_end:
			on_letter_cb(QR_DATA_MODE_END, 0, opaque);
			goto end;

		case QR_DATA_MODE_NUMERIC:
			len = bitstream_read_bits(r, LENGTH_BIT_SIZE_FOR_NUMERIC(data->version));
			if (len == 0) goto mode_end;
			while (len > 0) {
				uint16_t v = bitstream_read_bits(r, len >= 3 ? 10 : len == 2 ? 7 : 4);

				if (len >= 3) {
					if (v / 100 >= 10) {
						qrean_debug_printf("Warning: out of code founds on NUMERIC %d\n", v);
					}
					on_letter_cb(mode, '0' + v / 100 % 10, opaque);
					wrote++;
					len--;
				}
				if (len >= 2) {
					on_letter_cb(mode, '0' + v / 10 % 10, opaque);
					wrote++;
					len--;
				}
				on_letter_cb(mode, '0' + v % 10, opaque);
				wrote++;
				len--;
			}
			break;

		case QR_DATA_MODE_ALNUM:
			len = bitstream_read_bits(r, LENGTH_BIT_SIZE_FOR_ALNUM(data->version));
			if (len == 0) goto mode_end;
			while (len > 0) {
				uint16_t v = bitstream_read_bits(r, len >= 2 ? 11 : 6);
				if (len >= 2) {
					if (v / 45 >= 45) {
						qrean_debug_printf("Warning: out of code founds on ALNUM %d\n", v);
					}
					on_letter_cb(mode, alnum[v / 45 % 45], opaque);
					wrote++;
					if (--len == 0) break;
				}
				on_letter_cb(mode, alnum[v % 45], opaque);
				wrote++;
				if (--len == 0) break;
			}
			break;

		case QR_DATA_MODE_8BIT:
			len = bitstream_read_bits(r, LENGTH_BIT_SIZE_FOR_8BIT(data->version));
			if (len == 0) goto mode_end;
			while (len-- > 0) {
				on_letter_cb(mode, bitstream_read_bits(r, 8), opaque);
				wrote++;
			}
			break;

		case QR_DATA_MODE_ECI:;
			{
				uint_fast32_t eci;
				if (!bitstream_read_bit(r)) {
					// 0
					eci = bitstream_read_bits(r, 7);
				} else if (!bitstream_read_bit(r)) {
					// 10
					eci = bitstream_read_bits(r, 14);
				} else if (!bitstream_read_bit(r)) {
					// 110
					eci = bitstream_read_bits(r, 21);
				} else {
					// unsupported
					goto end;
				}

				on_letter_cb(mode, eci, opaque);

				// TODO:
			}
			break;
		case QR_DATA_MODE_KANJI:
			len = bitstream_read_bits(r, LENGTH_BIT_SIZE_FOR_8BIT(data->version));
			if (len == 0) goto mode_end;
			while (len-- > 0) {
				int code = bitstream_read_bits(r, 13);
				on_letter_cb(mode, code, opaque);
				wrote++;
			}
			break;

		case QR_DATA_MODE_STRUCTURED: {
			int a = bitstream_read_bits(r, 8);
			int b = bitstream_read_bits(r, 8);
			qrean_debug_printf("Warning: structured data %d %d\n", a, b);
		} break;

		default:
			goto end;
		}
	}

end:;
	return wrote;
}
