#ifndef __QR_VERSIONINFO_H__
#define __QR_VERSIONINFO_H__
#include "types.h"

typedef struct {
	qr_version_t version;
	uint32_t value;
} versioninfo_t;

#define VERSIONINFO_SIZE (18)

versioninfo_t create_versioninfo(qr_version_t version);

#endif /* __QR_VERSIONINFO_H__ */
