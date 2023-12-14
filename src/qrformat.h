#ifndef __QR_FORMATINFO_H__
#define __QR_FORMATINFO_H__

#include "qrtypes.h"

typedef struct {
	uint16_t value;
	qr_errorlevel_t level;
	qr_maskpattern_t mask;
} qrformat_t;

#define QR_FORMATINFO_SIZE (15)

qrformat_t qrformat_for(qr_errorlevel_t level, qr_maskpattern_t mask);
qrformat_t qrformat_from(uint16_t value);

#endif /* __QR_FORMATINFO_H__ */
