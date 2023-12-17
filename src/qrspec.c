#include <stdint.h>

uint_fast8_t qrspec_get_alignment_num(uint_fast8_t version) {
	if (version <= 1) return 0;
	int N = version / 7 + 2;
	return N * N - 3;
}

uint_fast8_t qrspec_get_alignment_steps(uint_fast8_t version, uint_fast8_t step) {
	if (version <= 1) return 0;
	uint_fast8_t N = version / 7 + 2;
	uint_fast8_t r = ((((version + 1) * 8 / (N - 1)) + 3) / 4) * 2 * (N - step - 1);
	uint_fast8_t v4 = version * 4;

	if (step >= N) return 0;

	return v4 < r ? 6 : v4 - r + 10;
}

uint_fast8_t qrspec_get_alignment_position_x(uint_fast8_t version, uint_fast8_t idx) {
	int N = version / 7 + 2;
	int xidx = (idx + 1) < (N - 1) * 1 ? (idx + 1) % N : (idx + 2) < (N - 1) * N ? (idx + 2) % N : (idx + 3) % N;
	return qrspec_get_alignment_steps(version, xidx);
}

uint_fast8_t qrspec_get_alignment_position_y(uint_fast8_t version, uint_fast8_t idx) {
	int N = version / 7 + 2;
	int yidx = (idx + 1) < (N - 1) * 1 ? (idx + 1) / N : (idx + 2) < (N - 1) * N ? (idx + 2) / N : (idx + 3) / N;
	return qrspec_get_alignment_steps(version, yidx);
}
