#include <assert.h>
#include <stdint.h>

#include "galois.h"
#include "qrversion.h"
#include "utils.h"

static const uint32_t bch[40] = {0x07c94, 0x085bc, 0x09a99, 0x0a4d3, 0x0bbf6, 0x0c762, 0x0d847, 0x0e60d, 0x0f928, 0x10b78, 0x1145d, 0x12a17,
                                 0x13532, 0x149a6, 0x15683, 0x168c9, 0x177ec, 0x18ec4, 0x191e1, 0x1afab, 0x1b08e, 0x1cc1a, 0x1d33f, 0x1ed75,
                                 0x1f250, 0x209d5, 0x216fd, 0x228ba, 0x2379f, 0x24b0b, 0x2542e, 0x26a64, 0x27541, 0x28c69};

qrversion_t qrversion_for(qr_version_t version) {
	assert(QR_VERSION_1 <= version && version <= QR_VERSION_40);

	qrversion_t vi;
	vi.version = version;
	vi.value = version >= 7 ? bch[version - 7] : 0xffffffff;
	return vi;
}

qrversion_t qrversion_from(uint32_t value) {
	qrversion_t vi = {
		.version = QR_VERSION_INVALID,
		.value = value,
	};

	for (int i = QR_VERSION_7; i < QR_VERSION_40; i++) {
		if (hamming_distance(bch[i - QR_VERSION_7], value) <= 3) {
			vi.version = (qr_version_t)i;
			vi.value = bch[i - QR_VERSION_7];
			return vi;
		}
	}

	return vi;
}
