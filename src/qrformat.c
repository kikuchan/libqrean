#include <assert.h>
#include <stdint.h>

#include "galois.h"
#include "qrbch.h"
#include "qrformat.h"
#include "utils.h"

#define NUM_MAPPINGS (8)
static const struct {
	qr_version_t version;
	qr_errorlevel_t level;
} mqr_mapping[NUM_MAPPINGS] = {
	{QR_VERSION_M1, QR_ERRORLEVEL_L},
	{QR_VERSION_M2, QR_ERRORLEVEL_L},
	{QR_VERSION_M2, QR_ERRORLEVEL_M},
	{QR_VERSION_M3, QR_ERRORLEVEL_L},
	{QR_VERSION_M3, QR_ERRORLEVEL_M},
	{QR_VERSION_M4, QR_ERRORLEVEL_L},
	{QR_VERSION_M4, QR_ERRORLEVEL_M},
	{QR_VERSION_M4, QR_ERRORLEVEL_Q},
};

qrformat_t qrformat_for(qr_version_t version, qr_errorlevel_t level, qr_maskpattern_t mask)
{
	assert(QR_ERRORLEVEL_L <= level && level <= QR_ERRORLEVEL_H);
	assert(QR_MASKPATTERN_0 <= mask && mask <= QR_MASKPATTERN_7);

	qrformat_t fi = {
		.version = QR_VERSION_INVALID,
		.mask = QR_MASKPATTERN_INVALID,
		.level = QR_ERRORLEVEL_INVALID,
		.value = 0xffffffff,
	};

	int idx = -1;
	if (IS_QR(version)) {
		idx = ((level ^ 1) * 8) | mask;
		fi.value = qrbch_15_5_value(idx);
	} else if (IS_MQR(version) && QR_MASKPATTERN_0 <= mask && mask <= QR_MASKPATTERN_3) {
		for (int i = 0; i < NUM_MAPPINGS; i++) {
			if (version == mqr_mapping[i].version && level == mqr_mapping[i].level) {
				idx = (i << 2) | mask;
				break;
			}
		}
		fi.value = qrbch_15_5_value(idx);
	} else if (IS_RMQR(version) && (level == QR_ERRORLEVEL_M || level == QR_ERRORLEVEL_H)) {
		idx = (level == QR_ERRORLEVEL_H ? (1 << 5) : 0) | (version - QR_VERSION_R7x43);
		fi.value = qrbch_18_6_value(idx);
	}

	if (idx >= 0) {
		fi.version = version;
		fi.mask = mask;
		fi.level = level;
	}

	return fi;
}

qrformat_t qrformat_from(qr_version_t version, uint32_t value)
{
	qrformat_t fi = {
		.version = QR_VERSION_INVALID,
		.mask = QR_MASKPATTERN_INVALID,
		.level = QR_ERRORLEVEL_INVALID,
		.value = value,
	};

	if (IS_RMQR(version)) {
		int i = qrbch_18_6_index_of(value);
		if (i >= 0) {
			fi.value = qrbch_18_6_value(i);

			fi.mask = QR_MASKPATTERN_0;
			fi.version = (qr_version_t)((i & 0b11111) + QR_VERSION_R7x43);
			fi.level = i & (1 << 5) ? QR_ERRORLEVEL_H : QR_ERRORLEVEL_M;
		}
	} else {
		int i = qrbch_15_5_index_of(value);
		if (i >= 0) {
			fi.value = qrbch_15_5_value(i);

			if (IS_QR(version)) {
				fi.mask = (qr_maskpattern_t)(i % 8 + QR_MASKPATTERN_0);
				fi.level = (qr_errorlevel_t)(((i / 8) ^ 1) + QR_ERRORLEVEL_L);
			} else if (IS_MQR(version)) {
				fi.mask = (qr_maskpattern_t)(i % 4 + QR_MASKPATTERN_0);
				fi.version = mqr_mapping[i / 4].version;
				fi.level = mqr_mapping[i / 4].level;
			}
		}
	}

	return fi;
}
