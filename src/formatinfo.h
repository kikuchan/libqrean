#ifndef __QR_FORMATINFO_H__
#define __QR_FORMATINFO_H__

#include "types.h"

typedef struct {
	uint16_t value;
	qr_errorlevel_t level;
	qr_maskpattern_t mask;
} formatinfo_t;

#define FORMATINFO_SIZE (15)

formatinfo_t create_formatinfo(qr_errorlevel_t level, qr_maskpattern_t mask);
formatinfo_t parse_formatinfo(uint16_t value);

#endif /* __QR_FORMATINFO_H__ */
