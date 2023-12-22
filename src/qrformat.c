#include <assert.h>
#include <stdint.h>

#include "galois.h"
#include "qrformat.h"
#include "qrtypes.h"
#include "utils.h"

// BCH (15, 5)
#define NUM_BCH (32)
static const uint16_t bch[NUM_BCH] = {
	0x0000, 0x0537, 0x0A6E, 0x0F59, 0x11EB, 0x14DC, 0x1B85, 0x1EB2, 0x23D6, 0x26E1, 0x29B8, 0x2C8F, 0x323D, 0x370A, 0x3853, 0x3D64,
	0x429B, 0x47AC, 0x48F5, 0x4DC2, 0x5370, 0x5647, 0x591E, 0x5C29, 0x614D, 0x647A, 0x6B23, 0x6E14, 0x70A6, 0x7591, 0x7AC8, 0x7FFF,
};

#define NUM_MAPPINGS (8)
static const struct {
	qr_version_t version;
	qr_errorlevel_t level;
} mapping[NUM_MAPPINGS] = {
	{QR_VERSION_M1, QR_ERRORLEVEL_L},
    {QR_VERSION_M2, QR_ERRORLEVEL_L},
    {QR_VERSION_M2, QR_ERRORLEVEL_M},
    {QR_VERSION_M3, QR_ERRORLEVEL_L},
	{QR_VERSION_M3, QR_ERRORLEVEL_M},
    {QR_VERSION_M4, QR_ERRORLEVEL_L},
    {QR_VERSION_M4, QR_ERRORLEVEL_M},
    {QR_VERSION_M4, QR_ERRORLEVEL_Q},
};

qrformat_t qrformat_for(qr_version_t version, qr_errorlevel_t level, qr_maskpattern_t mask) {
	assert(QR_ERRORLEVEL_L <= level && level <= QR_ERRORLEVEL_H);
	assert(QR_MASKPATTERN_0 <= mask && mask <= QR_MASKPATTERN_7);

	qrformat_t fi = {
		.version = QR_VERSION_INVALID,
		.mask = QR_MASKPATTERN_INVALID,
		.level = QR_ERRORLEVEL_INVALID,
		.value = 0xffff,
	};

	int idx = -1;
	if (IS_QR(version)) {
		idx = ((level ^ 1) * 8) | mask;
	} else if (IS_MQR(version) && QR_MASKPATTERN_0 <= mask && mask <= QR_MASKPATTERN_3) {
		for (int i = 0; i < NUM_MAPPINGS; i++) {
			if (version == mapping[i].version && level == mapping[i].level) {
				idx = (i << 2) | mask;
				break;
			}
		}
	}

	if (0 <= idx && idx <= NUM_BCH) {
		fi.value = bch[idx];
		fi.version = version;
		fi.mask = mask;
		fi.level = level;
	}

	return fi;
}

qrformat_t qrformat_from(qr_version_t version, uint16_t value) {
	qrformat_t fi = {
		.version = QR_VERSION_INVALID,
		.mask = QR_MASKPATTERN_INVALID,
		.level = QR_ERRORLEVEL_INVALID,
		.value = value,
	};

	for (int i = 0; i < NUM_BCH; i++) {
		if (hamming_distance(bch[i], value) <= 3) {
			fi.value = bch[i];

			if (IS_QR(version)) {
				fi.mask = (qr_maskpattern_t)(i % 8 + QR_MASKPATTERN_0);
				fi.level = (qr_errorlevel_t)(i / 8 + QR_ERRORLEVEL_L);
			} else {
				fi.mask = (qr_maskpattern_t)(i % 4 + QR_MASKPATTERN_0);
				fi.version = mapping[i / 4].version;
				fi.level = mapping[i / 4].level;
			}
			return fi;
		}
	}
	return fi;
}
