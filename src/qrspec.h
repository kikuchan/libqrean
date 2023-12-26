#ifndef __QR_QRSPEC_H__
#define __QR_QRSPEC_H__

#include <stdint.h>
#include <stdio.h>

#include "qrtypes.h"

uint_fast8_t qrspec_get_symbol_width(qr_version_t version);
uint_fast8_t qrspec_get_symbol_height(qr_version_t version);

uint_fast8_t qrspec_get_alignment_num(qr_version_t version);
uint_fast8_t qrspec_get_alignment_steps(qr_version_t version, uint_fast8_t step);
uint_fast8_t qrspec_get_alignment_position_x(qr_version_t version, uint_fast8_t idx);
uint_fast8_t qrspec_get_alignment_position_y(qr_version_t version, uint_fast8_t idx);

size_t qrspec_get_available_bits(qr_version_t version);
uint_fast8_t qrspec_get_total_blocks(qr_version_t version, qr_errorlevel_t level);
uint_fast8_t qrspec_get_error_words_in_block(qr_version_t version, qr_errorlevel_t level);

uint_fast8_t qrspec_get_data_bitlength_for(qr_version_t version, int mode);

const char *qrspec_get_version_string(qr_version_t version);
qr_version_t qrspec_get_version_by_string(const char *version);

int qrspec_is_valid_combination(qr_version_t version, qr_errorlevel_t level, qr_maskpattern_t mask);

#endif /* __QR_QRSPEC_H__ */
