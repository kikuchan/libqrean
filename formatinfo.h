#include "types.h"

typedef struct {
	uint16_t value;
} formatinfo_t;

#define FORMATINFO_SIZE (15)

formatinfo_t create_formatinfo(qr_errorlevel_t level, qr_maskpattern_t mask);


