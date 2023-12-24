#include <assert.h>
#include <stdint.h>

#include "utils.h"

#define NUM_BCH_15_5 (32)
static const uint16_t bch_15_5[NUM_BCH_15_5] = {
	0x0000, 0x0537, 0x0A6E, 0x0F59, 0x11EB, 0x14DC, 0x1B85, 0x1EB2, 0x23D6, 0x26E1, 0x29B8, 0x2C8F, 0x323D, 0x370A, 0x3853, 0x3D64,
	0x429B, 0x47AC, 0x48F5, 0x4DC2, 0x5370, 0x5647, 0x591E, 0x5C29, 0x614D, 0x647A, 0x6B23, 0x6E14, 0x70A6, 0x7591, 0x7AC8, 0x7FFF,
};

#define NUM_BCH_18_6 (64)
static const uint32_t bch_18_6[] = {
	0x00000, 0x01f25, 0x0216f, 0x03e4a, 0x042de, 0x05dfb, 0x063b1, 0x07c94, 0x085bc, 0x09a99, 0x0a4d3, 0x0bbf6, 0x0c762,
	0x0d847, 0x0e60d, 0x0f928, 0x10b78, 0x1145d, 0x12a17, 0x13532, 0x149a6, 0x15683, 0x168c9, 0x177ec, 0x18ec4, 0x191e1,
	0x1afab, 0x1b08e, 0x1cc1a, 0x1d33f, 0x1ed75, 0x1f250, 0x209d5, 0x216f0, 0x228ba, 0x2379f, 0x24b0b, 0x2542e, 0x26a64,
	0x27541, 0x28c69, 0x2934c, 0x2ad06, 0x2b223, 0x2ceb7, 0x2d192, 0x2efd8, 0x2f0fd, 0x302ad, 0x31d88, 0x323c2, 0x33ce7,
	0x34073, 0x35f56, 0x3611c, 0x37e39, 0x38711, 0x39834, 0x3a67e, 0x3b95b, 0x3c5cf, 0x3daea, 0x3e4a0, 0x3fb85,
};

uint16_t qrbch_15_5_value(uint_fast8_t index) {
	assert(index < NUM_BCH_15_5);
	return bch_15_5[index];
}

uint32_t qrbch_18_6_value(uint_fast8_t index) {
	assert(index < NUM_BCH_18_6);
	return bch_18_6[index];
}

int qrbch_15_5_index_of(uint16_t value) {
	for (int i = 0; i < NUM_BCH_15_5; i++) {
		if (hamming_distance(bch_15_5[i], value) <= 3) return i;
	}
	return -1;
}

int qrbch_18_6_index_of(uint32_t value) {
	for (int i = 0; i < NUM_BCH_18_6; i++) {
		if (hamming_distance(bch_18_6[i], value) <= 3) return i;
	}
	return -1;
}
