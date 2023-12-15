#ifndef __QR_QRVERSION_H__
#define __QR_QRVERSION_H__
#include "qrtypes.h"

typedef struct {
	qr_version_t version;
	uint32_t value;
} qrversion_t;

#define QR_VERSIONINFO_SIZE (18)

qrversion_t qrversion_for(qr_version_t version);
qrversion_t qrversion_from(uint32_t version);

#endif /* __QR_QRVERSION_H__ */
