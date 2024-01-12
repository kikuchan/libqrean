#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "utils.h"

#ifndef NO_KANJI_TABLE
#include "qrkanji-table.h"

static uint16_t read_16le(uint8_t *buf)
{
	return buf[0] | (buf[1] << 8);
}

static int search_trie(uint8_t *root, const char *word, uint16_t *data)
{
	uint8_t *temp = root;
	for (int i = 0; word[i]; i++) {
		int n = *temp++;
		uint8_t *p = (uint8_t *)memchr((char *)temp, word[i], n);
		if (!p) return -1;

		int idx = (p - temp);
		temp += n;

		int is_leaf = temp[idx / 8] & (0x80 >> (idx % 8));
		temp += (n + 7) / 8;

		if (is_leaf) {
			if (data) *data = read_16le(temp + idx * 2);
			return i + 1;
		}

		temp += n * 2 + read_16le(temp + idx * 2);
	}
	return -1;
}

#endif

int_fast16_t qrkanji_index_from_utf8(const char *str, size_t *consumed)
{
#ifndef NO_KANJI_TABLE
	uint16_t data;
	int len = search_trie(utf8_to_qrkanji_trie, str, &data);
	if (len < 0) return -1;
	if (consumed) *consumed = len;
	return data;
#else
	return -1;
#endif
}

uint16_t qrkanji_index_to_unicode(int_fast16_t idx)
{
#ifndef NO_KANJI_TABLE
	if (idx < 0 || (size_t)idx > sizeof(cp932_to_unicode_tbl) / sizeof(cp932_to_unicode_tbl[0])) return 0;
	return cp932_to_unicode_tbl[idx];
#endif
	return 0;
}

int_fast16_t qrkanji_index_from_sjis(const char *str, size_t *consumed)
{
	int hi = (uint8_t)str[0] & 0xff;
	int lo = (uint8_t)str[1] & 0xff;

	if (hi >= 0x81 && hi <= 0x9f) {
		hi -= 0x81; // the offset defined by QR spec
	} else if (hi >= 0xe0 && hi <= 0xfc) {
		hi -= 0xc1; // the offset defined by QR spec
	} else {
		return -1;
	}
	if (lo >= 0x40 && lo <= 0xfc) {
		lo -= 0x40;
	} else {
		return -1;
	}

	if (consumed) *consumed = 2;
	return hi * 0xc0 + lo;
}

uint16_t qrkanji_index_to_sjis(int_fast16_t idx)
{
	int hi;
	int lo;

	hi = idx / 0xc0;
	lo = idx % 0xc0;

	if (hi < 0) {
		return 0;
	} else if (hi < 0x1f) {
		hi += 0x81;
	} else if (hi < 0x2b) {
		hi += 0xc1;
	} else if (hi < 0x2d) {
		hi += 0xc2;
	} else if (hi < 0x30) {
		hi += 0xcd;
	} else {
		return 0;
	}

	lo += 0x40;

	return (hi << 8) | lo;
}

uint16_t qrkanji_sjis_to_unicode(uint8_t sjis1, uint8_t sjis2, size_t *consumed)
{
	// HALFWIDTH KATAKANA
	if (0xa1 <= sjis1 && sjis1 <= 0xdf) {
		if (consumed) *consumed = 1;
		return 0xff61 + sjis1 - 0xa1;
	}

	uint8_t buf[2] = { sjis1, sjis2 };
	int_fast16_t idx = qrkanji_index_from_sjis((const char *)buf, NULL);
	if (idx < 0) {
		// ASCII or Invalid SJIS, fallback to Latin1
		if (consumed) *consumed = 1;
		return sjis1;
	}
	if (consumed) *consumed = 2;

	uint16_t uni = qrkanji_index_to_unicode(idx);
	if (!uni) uni = 0x3013; // 〓
	return uni;
}

int qrkanji_utf8_to_unicode(const char *src, size_t *consumed)
{
	int uni = '?';
	if (consumed) *consumed = 1;
	if ((uint8_t)*src < 0x80) {
		uni = *src;
		if (consumed) *consumed = 1;
	} else if (((uint8_t)src[0] & 0xe0) == 0xc0) {
		if (((uint8_t)src[1] & 0xc0) != 0x80) return uni;
		uni = (src[0] & 0x1f) << 6 | (src[1] & 0x3f);
		if (consumed) *consumed = 2;
	} else if (((uint8_t)src[0] & 0xf0) == 0xe0) {
		if (((uint8_t)src[1] & 0xc0) != 0x80 || ((uint8_t)src[2] & 0xc0) != 0x80) return uni;
		uni = (src[0] & 0x0f) << 12 | (src[1] & 0x3f) << 6 | (src[2] & 0x3f);
		if (consumed) *consumed = 3;
	} else if (((uint8_t)src[0] & 0xf8) == 0xf0) {
		if (((uint8_t)src[1] & 0xc0) != 0x80 || ((uint8_t)src[2] & 0xc0) != 0x80 || ((uint8_t)src[3] & 0xc0) != 0x80) return uni;
		uni = (src[0] & 0x07) << 18 | (src[1] & 0x3f) << 12 | (src[2] & 0x3f) << 6 | (src[3] & 0x3f);
		if (consumed) *consumed = 4;
	}

	return uni;
}

static size_t unicode_to_utf8(uint32_t code, char *dst)
{
	if (code < 0x80) {
		dst[0] = code;
		dst[1] = 0;
		return 1;
	} else if (code < 0x800) {
		dst[0] = 0xc0 | (code >> 6);
		dst[1] = 0x80 | (code & 0x3f);
		dst[2] = 0;
		return 2;
	} else if (code < 0x10000) {
		dst[0] = 0xe0 | (code >> 12);
		dst[1] = 0x80 | ((code >> 6) & 0x3f);
		dst[2] = 0x80 | (code & 0x3f);
		dst[3] = 0;
		return 3;
	} else if (code < 0x200000) {
		dst[0] = 0xf0 | (code >> 18);
		dst[1] = 0x80 | ((code >> 12) & 0x3f);
		dst[2] = 0x80 | ((code >> 6) & 0x3f);
		dst[3] = 0x80 | (code & 0x3f);
		dst[4] = 0;
		return 4;
	} else {
		// invalid
		dst[0] = 0;
		return 0;
	}
}

static size_t unicode_to_sjis(uint32_t uni, char *dst)
{
	// ASCII
	if (uni < 0x80) {
		dst[0] = uni;
		dst[1] = 0;
		return 1;
	}

	// HALFWIDTH KATAKANA
	if (0xff61 <= uni && uni <= 0xff9f) {
		dst[0] = 0xa1 + (uni - 0xff61);
		dst[1] = 0;
		return 1;
	}

	// XXX: back to UTF8 again. since, the conversion table requires UTF8 string
	char buf[5] = {};
	unicode_to_utf8(uni, buf);
	int idx = qrkanji_index_from_utf8(buf, NULL);
	uint16_t sjis = qrkanji_index_to_sjis(idx);
	if (sjis == 0) {
		dst[0] = '?';
		dst[1] = 0;
		return 1;
	}

	dst[0] = (sjis >> 8);
	dst[1] = (sjis & 0xff);
	dst[2] = 0;

	return 2;
}

size_t qrkanji_sjis_to_utf8(char *dst, size_t dstlen, const char *src, size_t srclen)
{
	const char *p = src;
	char *q = dst;
	char buf[5] = {};

	while (*p && p < src + srclen && q < dst + dstlen - 1) {
		size_t consumed;
		uint16_t uni = qrkanji_sjis_to_unicode(p[0], p[1], &consumed);
		p += consumed;

		size_t l = unicode_to_utf8(uni, buf);
		if (q + l < dst + dstlen - 1) {
			unicode_to_utf8(uni, q);
			q += l;
		}
	}
	*q = '\0';

	return q - dst;
}

size_t qrkanji_utf8_to_sjis(char *dst, size_t dstlen, const char *src, size_t srclen)
{
	const char *p = src;
	char *q = dst;
	char buf[3] = {};

	while (*p && p < src + srclen && q < dst + dstlen - 1) {
		size_t consumed;
		int uni = qrkanji_utf8_to_unicode(p, &consumed);
		p += consumed;

		size_t l = unicode_to_sjis(uni, buf);
		if (q + l < dst + dstlen - 1) {
			unicode_to_sjis(uni, q);
			q += l;
		}
	}
	*q = '\0';

	return q - dst;
}
