#include <assert.h>
#include <stdint.h>

#include "formatinfo.h"

static const uint16_t bch[] = {
	0x23D6, 0x26E1, 0x29B8, 0x2C8F, 0x323D, 0x370A, 0x3853, 0x3D64, // L
	0x0000, 0x0537, 0x0A6E, 0x0F59, 0x11EB, 0x14DC, 0x1B85, 0x1EB2, // M
	0x614D, 0x647A, 0x6B23, 0x6E14, 0x70A6, 0x7591, 0x7AC8, 0x7FFF, // Q
	0x429B, 0x47AC, 0x48F5, 0x4DC2, 0x5370, 0x5647, 0x591E, 0x5C29, // H
};

formatinfo_t create_formatinfo(qr_errorlevel_t level, qr_maskpattern_t mask) {
	assert(QR_ERRORLEVEL_L <= level && level <= QR_ERRORLEVEL_H);
	assert(QR_MASKPATTERN_0 <= mask && mask <= QR_MASKPATTERN_7);

	formatinfo_t fi;
	fi.value = bch[(level * 8) | mask];
	fi.mask = mask;
	fi.level = level;
	return fi;
}

#include <stdio.h>
formatinfo_t parse_formatinfo(uint16_t value) {
	for (int i = 0; i < 8 * 4; i++) {
		if (bch[i] == value) {
			formatinfo_t fi;
			// XXX:
			fi.mask = (qr_maskpattern_t)(i % 8);
			fi.level = (qr_errorlevel_t)(i / 8);
			fi.value = bch[(fi.level * 8) | fi.mask];
			return fi;
		}
	}

	formatinfo_t fi;
	fi.mask = QR_MASKPATTERN_INVALID;
	fi.level = QR_ERRORLEVEL_INVALID;
	fi.value = value;
	return fi;
}
