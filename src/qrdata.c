#include <string.h>

#include "bitstream.h"
#include "debug.h"
#include "qrdata.h"
#include "qrkanji.h"
#include "qrspec.h"
#include "utils.h"

qrdata_t create_qrdata_for(bitstream_t bs, qr_version_t version, qr_eci_code_t eci_code)
{
	qrdata_t data = {
		.bs = bs,
		.version = version,
		.eci_code = eci_code,
		.eci_last = QR_ECI_CODE_LATIN1,
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

static size_t measure_kanji(const char *src, size_t *byte_consumed, qr_eci_code_t code)
{
	const char *ptr = src;
	int len = 0;

	int (*qrkanji_lookup)(const char *str, size_t *consumed);
	if (code == QR_ECI_CODE_UTF8) {
		qrkanji_lookup = qrkanji_index_from_utf8;
	} else if (code == QR_ECI_CODE_SJIS) {
		qrkanji_lookup = qrkanji_index_from_sjis;
	} else {
		// Not supported
		return 0;
	}

	while (*ptr) {
		size_t consumed;
		int idx = qrkanji_lookup(ptr, &consumed);
		if (idx < 0 || idx >= 0x1fff) break;
		ptr += consumed;
		len++;
	}

	if (byte_consumed) *byte_consumed = ptr - src;
	return len;
}

#define LEN_CMP_8BIT(ch)             (strcspn((ch), alnum))
#define LEN_CMP_ALNUM(ch)            (strspn((ch), alnum_cmp))
#define LEN_NUMERIC(ch)              (measure_numeric(ch))
#define LEN_ALNUM(ch)                (measure_alnum(ch))
#define LEN_KANJI(ch, bytelen, data) (measure_kanji((ch), (bytelen), (data)->eci_code))

#define MQR_DATA_MODE_NUMERIC (0)
#define MQR_DATA_MODE_ALNUM   (1)
#define MQR_DATA_MODE_8BIT    (2)
#define MQR_DATA_MODE_KANJI   (3)

#define RMQR_DATA_MODE_END     (0)
#define RMQR_DATA_MODE_NUMERIC (1)
#define RMQR_DATA_MODE_ALNUM   (2)
#define RMQR_DATA_MODE_8BIT    (3)
#define RMQR_DATA_MODE_KANJI   (4)

bitpos_t qrdata_write_eci_code(qrdata_t *data, qr_eci_code_t eci)
{
	if (eci < 0) return 0;

	if (IS_MQR(data->version)) {
		// unsupported
		return 0;
	} else if (IS_RMQR(data->version)) {
		// Not supported yet
		// bitstream_write_bits(&data->bs, RMQR_DATA_MODE_ECI, 3);
		return 0;
	} else if (IS_QR(data->version)) {
		bitstream_write_bits(&data->bs, QR_DATA_MODE_ECI, 4);
	} else {
		return 0;
	}

	if (eci <= 127) {
		bitstream_write_bits(&data->bs, eci, 8);
		return 4 + 8;
	} else if (eci <= 16383) {
		bitstream_write_bits(&data->bs, 0b10, 2);
		bitstream_write_bits(&data->bs, eci, 14);
		return 4 + 16;
	} else if (eci <= 999999) {
		bitstream_write_bits(&data->bs, 0b100, 3);
		bitstream_write_bits(&data->bs, eci, 21);
		return 4 + 24;
	}
	return 0;
}

size_t qrdata_write_numeric_string(qrdata_t *data, const char *src, size_t len)
{
	if (measure_numeric(src) < len) {
		return 0;
	}

	if (IS_MQR(data->version)) {
		bitstream_write_bits(&data->bs, MQR_DATA_MODE_NUMERIC, data->version - QR_VERSION_M1);
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
		bitstream_write_bits(&data->bs, MQR_DATA_MODE_ALNUM, data->version - QR_VERSION_M1);
	} else if (IS_RMQR(data->version)) {
		bitstream_write_bits(&data->bs, RMQR_DATA_MODE_ALNUM, 3);
	} else if (IS_QR(data->version)) {
		bitstream_write_bits(&data->bs, QR_DATA_MODE_ALNUM, 4);
	} else {
		// unsupported
		return 0;
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

	if (data->eci_last != data->eci_code) {
		qrdata_write_eci_code(data, data->eci_code);
		data->eci_last = data->eci_code;
	}

	if (IS_MQR(data->version)) {
		if (data->version <= QR_VERSION_M2) return 0;
		bitstream_write_bits(&data->bs, MQR_DATA_MODE_8BIT, data->version - QR_VERSION_M1);
	} else if (IS_RMQR(data->version)) {
		bitstream_write_bits(&data->bs, RMQR_DATA_MODE_8BIT, 3);
	} else if (IS_QR(data->version)) {
		bitstream_write_bits(&data->bs, QR_DATA_MODE_8BIT, 4);
	} else {
		// unsupported
		return 0;
	}

	bitstream_write_bits(&data->bs, len, LENGTH_BIT_SIZE_FOR_8BIT(data->version));

	size_t i;
	for (i = 0; i < len && !bitstream_is_end(&data->bs); i++) {
		bitstream_write_bits(&data->bs, src[i], 8);
	}
	return i;
}

size_t qrdata_write_kanji_string(qrdata_t *data, const char *src, size_t srclen)
{
	size_t bytelen = 0;
	size_t kanjilen = measure_kanji(src, &bytelen, data->eci_code);
	if (bytelen < srclen) return 0;

	if (IS_MQR(data->version)) {
		if (data->version <= QR_VERSION_M2) return 0;
		bitstream_write_bits(&data->bs, MQR_DATA_MODE_KANJI, data->version - QR_VERSION_M1);
	} else if (IS_RMQR(data->version)) {
		bitstream_write_bits(&data->bs, RMQR_DATA_MODE_KANJI, 3);
	} else if (IS_QR(data->version)) {
		bitstream_write_bits(&data->bs, QR_DATA_MODE_KANJI, 4);
	} else {
		// unsupported
		return 0;
	}

	bitstream_write_bits(&data->bs, kanjilen, LENGTH_BIT_SIZE_FOR_KANJI(data->version));

	int (*qrkanji_lookup)(const char *str, size_t *consumed);
	if (data->eci_code == QR_ECI_CODE_UTF8) {
		qrkanji_lookup = qrkanji_index_from_utf8;
	} else if (data->eci_code == QR_ECI_CODE_SJIS) {
		qrkanji_lookup = qrkanji_index_from_sjis;
	} else {
		// Not supported
		return 0;
	}

	size_t i;
	size_t consumed = 0;
	for (i = 0; i < srclen && !bitstream_is_end(&data->bs); i += consumed) {
		int_fast16_t idx = qrkanji_lookup(src + i, &consumed);
		if (idx < 0 || idx >= 0x1fff) break; // XXX:

		bitstream_write_bits(&data->bs, idx, 13);
	}
	return i;
}

bit_t qrdata_finalize(qrdata_t *data)
{
	if (IS_MQR(data->version)) {
		if (!bitstream_write_bits(&data->bs, 0, 3 + 2 * (data->version - QR_VERSION_M1))) return 0;
	} else if (IS_RMQR(data->version)) {
		if (!bitstream_write_bits(&data->bs, RMQR_DATA_MODE_END, 3)) return 0;
	} else if (IS_QR(data->version)) {
		if (!bitstream_write_bits(&data->bs, QR_DATA_MODE_END, 4)) return 0;
	} else if (IS_TQR(data->version)) {
		if (bitstream_tell(&data->bs) != 4 * 10) {
			return 0;
		}
	} else {
		return 0;
	}
	if (bitstream_tell(&data->bs) % 8) bitstream_write_bits(&data->bs, 0, 8 - (bitstream_tell(&data->bs) % 8));

	size_t pos = bitstream_tell(&data->bs);

	int flip = 0;
	for (; pos < data->bs.size / 8 * 8; pos += 8) {
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

	case QR_DATA_MODE_KANJI:
		return qrdata_write_kanji_string(data, src, len);

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
	if (LEN_KANJI(src, NULL, data) > 0) {
		mode = QR_DATA_MODE_KANJI;
	} else if (LEN_CMP_8BIT(src) > 0) {
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

	i = 0;
	while (i < len) {
		size_t bytelen;
		if (LEN_KANJI(src + i, &bytelen, data) > 0) {
			mode = QR_DATA_MODE_KANJI;
		} else {
			bytelen = 1;
			if (mode == QR_DATA_MODE_KANJI || mode == QR_DATA_MODE_8BIT) {
				mode = QR_DATA_MODE_8BIT;
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
		}

		if (mode != last_mode) {
			size_t l = qrdata_flush(data, last_mode, src + last_i, i - last_i);
			if (last_i + l < i) return last_i + l;
			last_mode = mode;
			last_i = i;
		}

		i += bytelen;
	}

	size_t r = last_i + qrdata_flush(data, last_mode, src + last_i, len - last_i);
	return r;
}

size_t qrdata_parse(qrdata_t *data, qrdata_parse_callback_t on_letter_cb, void *opaque)
{
	bitstream_t *r = &data->bs;
	size_t len;
	size_t wrote = 0;
	qr_eci_code_t eci = data->eci_code;

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
			wrote += on_letter_cb(QR_DATA_LETTER_TYPE_END, 0, opaque);
			goto end;

		case QR_DATA_MODE_NUMERIC:
			len = bitstream_read_bits(r, LENGTH_BIT_SIZE_FOR_NUMERIC(data->version));
			if (IS_TQR(data->version)) len = 12;
			if (len == 0) goto mode_end;
			while (len > 0) {
				uint16_t v = bitstream_read_bits(r, len >= 3 ? 10 : len == 2 ? 7 : 4);

				if (len >= 3) {
					if (v / 100 >= 10) {
						qrean_debug_printf("Warning: out of code founds on NUMERIC %d\n", v);
					}
					wrote += on_letter_cb(QR_DATA_LETTER_TYPE_RAW, '0' + v / 100 % 10, opaque);
					len--;
				}
				if (len >= 2) {
					wrote += on_letter_cb(QR_DATA_LETTER_TYPE_RAW, '0' + v / 10 % 10, opaque);
					len--;
				}
				wrote += on_letter_cb(QR_DATA_LETTER_TYPE_RAW, '0' + v % 10, opaque);
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
					wrote += on_letter_cb(QR_DATA_LETTER_TYPE_RAW, alnum[v / 45 % 45], opaque);
					if (--len == 0) break;
				}
				wrote += on_letter_cb(QR_DATA_LETTER_TYPE_RAW, alnum[v % 45], opaque);
				if (--len == 0) break;
			}
			break;

		case QR_DATA_MODE_8BIT:
			len = bitstream_read_bits(r, LENGTH_BIT_SIZE_FOR_8BIT(data->version));
			if (len == 0) goto mode_end;

			while (len > 0) {
				int32_t ch;

#ifndef NO_KANJI_TABLE
				if (eci == QR_ECI_CODE_UTF8) {
					size_t consumed;
					ch = bitstream_read_unicode_from_utf8(r, len, &consumed);
					len -= consumed;
					if (ch > 0) {
						wrote += on_letter_cb(QR_DATA_LETTER_TYPE_UNICODE, ch, opaque);
					}
				} else if (eci == QR_ECI_CODE_SJIS) {
					ch = bitstream_read_bits(r, 8);
					uint8_t next = bitstream_peek_bits(r, 8);
					size_t consumed;
					ch = qrkanji_sjis_to_unicode(ch, next, &consumed);
					if (consumed == 2) {
						bitstream_skip_bits(r, 8);
					}
					len -= consumed;

					wrote += on_letter_cb(QR_DATA_LETTER_TYPE_UNICODE, ch, opaque);
				} else {
#endif
					ch = bitstream_read_bits(r, 8);
					len--;
					wrote += on_letter_cb(QR_DATA_LETTER_TYPE_RAW, ch, opaque);
#ifndef NO_KANJI_TABLE
				}
#endif
			}
			break;

		case QR_DATA_MODE_KANJI:
			len = bitstream_read_bits(r, LENGTH_BIT_SIZE_FOR_KANJI(data->version));
			if (len == 0) goto mode_end;
			while (len-- > 0) {
#ifndef NO_KANJI_TABLE
				uint16_t uni = qrkanji_index_to_unicode(bitstream_read_bits(r, 13));
				if (uni) wrote += on_letter_cb(QR_DATA_LETTER_TYPE_UNICODE, uni, opaque);
#else
				uint16_t sjis = qrkanji_index_to_sjis(bitstream_read_bits(r, 13));
				if (sjis) {
					wrote += on_letter_cb(QR_DATA_LETTER_TYPE_RAW, (sjis >> 8) & 0xff, opaque);
					wrote += on_letter_cb(QR_DATA_LETTER_TYPE_RAW, (sjis >> 0) & 0xff, opaque);
				}
#endif
			}
			break;

		case QR_DATA_MODE_ECI: {
			if (!bitstream_read_bit(r)) {
				// 0
				eci = (qr_eci_code_t)bitstream_read_bits(r, 7);
			} else if (!bitstream_read_bit(r)) {
				// 10
				eci = (qr_eci_code_t)bitstream_read_bits(r, 14);
			} else if (!bitstream_read_bit(r)) {
				// 110
				eci = (qr_eci_code_t)bitstream_read_bits(r, 21);
			} else {
				// unsupported
				goto end;
			}
			wrote += on_letter_cb(QR_DATA_LETTER_TYPE_ECI_CHANGE, eci, opaque);
		} break;

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
