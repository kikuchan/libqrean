#ifndef __QR_QRSPEC_H__
#define __QR_QRSPEC_H__

#include <stdint.h>
uint_fast8_t qrspec_get_alignment_num(uint_fast8_t version);
uint_fast8_t qrspec_get_alignment_steps(uint_fast8_t version, uint_fast8_t step);
uint_fast8_t qrspec_get_alignment_position_x(uint_fast8_t version, uint_fast8_t idx);
uint_fast8_t qrspec_get_alignment_position_y(uint_fast8_t version, uint_fast8_t idx);

#endif /* __QR_QRSPEC_H__ */
