#ifndef __QR_QRSPEC_H__
#define __QR_QRSPEC_H__

#include <stdio.h>
#include <stdint.h>

#include "qrtypes.h"

uint_fast8_t qrspec_get_alignment_num(uint_fast8_t version);
uint_fast8_t qrspec_get_alignment_steps(uint_fast8_t version, uint_fast8_t step);
uint_fast8_t qrspec_get_alignment_position_x(uint_fast8_t version, uint_fast8_t idx);
uint_fast8_t qrspec_get_alignment_position_y(uint_fast8_t version, uint_fast8_t idx);

size_t qrspec_get_available_bits(uint_fast8_t version);
uint_fast8_t qrspec_get_total_blocks(qr_version_t version, qr_errorlevel_t level);
uint_fast8_t qrspec_get_error_words_in_block(qr_version_t version, qr_errorlevel_t level);

#endif /* __QR_QRSPEC_H__ */
