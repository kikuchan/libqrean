#ifndef __QREAN_QRKANJI_H__
#define __QREAN_QRKANJI_H__

#include <stddef.h>
#include <stdint.h>

// returns byte consumed, -1 for not found
int qrkanji_from_utf8(const char *str, uint16_t *data);

// write utf8 on dst, returns unicode point
uint16_t qrkanji_to_utf8(uint16_t data, char *dst);

#endif /* __QREAN_QRKANJI_H__ */
