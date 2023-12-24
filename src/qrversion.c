#include <assert.h>
#include <stdint.h>

#include "galois.h"
#include "qrbch.h"
#include "qrversion.h"
#include "utils.h"

qrversion_t qrversion_for(qr_version_t version) {
	assert(IS_QR(version) || IS_MQR(version) || IS_RMQR(version));

	qrversion_t vi;
	vi.version = version;
	vi.value = QR_VERSION_7 <= version && version <= QR_VERSION_40 ? qrbch_18_6_value(version) : 0xffffffff;
	return vi;
}

qrversion_t qrversion_from(uint32_t value) {
	qrversion_t vi = {
		.version = QR_VERSION_INVALID,
		.value = value,
	};

	int i = qrbch_18_6_index_of(value);
	if (QR_VERSION_7 <= i && i <= QR_VERSION_40) {
		vi.version = (qr_version_t)i;
		vi.value = qrbch_18_6_value(i);
		return vi;
	}

	return vi;
}
