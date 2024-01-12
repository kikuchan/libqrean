#ifndef __QRKANJI_H__
#define __QRKANJI_H__

#include <stddef.h>
#include <stdint.h>

// Low-level APIs

// returns qrkanji index, -1 for not found
int qrkanji_index_from_utf8(const char *str, size_t *consumed);

// returns qrkanji index, -1 for not found
int qrkanji_index_from_sjis(const char *str, size_t *consumed);

// returns Unicode code point, 0 for not found
uint16_t qrkanji_index_to_unicode(int_fast16_t data);

// returns SJIS code point, 0 for not found
uint16_t qrkanji_index_to_sjis(int_fast16_t idx);

// returns Unicode code point
uint16_t qrkanji_sjis_to_unicode(uint8_t sjis1, uint8_t sjis2, size_t *consumed);

// High-level APIs

// convert SJIS string to UTF8
size_t qrkanji_sjis_to_utf8(char *dst, size_t dstlen, const char *src, size_t srclen);
// convert UTF8 string to SJIS
size_t qrkanji_utf8_to_sjis(char *dst, size_t dstlen, const char *src, size_t srclen);

#endif /* __QRKANJI_H__ */
