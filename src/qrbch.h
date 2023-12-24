#ifndef __QR_QRBCH_H__
#define __QR_QRBCH_H__

#include <stdint.h>

uint32_t qrbch_15_5_value(uint_fast8_t index);
uint32_t qrbch_18_6_value(uint_fast8_t index);
int qrbch_15_5_index_of(uint32_t value);
int qrbch_18_6_index_of(uint32_t value);

#endif /* __QR_QRBCH_H__ */
