#include "types.h"

typedef struct {
	uint32_t value;
} versioninfo_t;

#define VERSIONINFO_SIZE (18)

versioninfo_t create_versioninfo(qr_version_t version);
