#include <stdio.h>
#include <stdint.h>

#include "qrtypes.h"

static const uint8_t QR_ERROR_WORDS_IN_BLOCK[40][4] = {
  //  L,  M,  Q,  H
	{ 7, 10, 13, 17}, //  1
	{10, 16, 22, 28}, //  2
	{15, 26, 18, 22}, //  3
	{20, 18, 26, 16}, //  4
	{26, 24, 18, 22}, //  5
	{18, 16, 24, 28}, //  6
	{20, 18, 18, 26}, //  7
	{24, 22, 22, 26}, //  8
	{30, 22, 20, 24}, //  9
	{18, 26, 24, 28}, // 10
	{20, 30, 28, 24}, // 11
	{24, 22, 26, 28}, // 12
	{26, 22, 24, 22}, // 13
	{30, 24, 20, 24}, // 14
	{22, 24, 30, 24}, // 15
	{24, 28, 24, 30}, // 16
	{28, 28, 28, 28}, // 17
	{30, 26, 28, 28}, // 18
	{28, 26, 26, 26}, // 19
	{28, 26, 30, 28}, // 20
	{28, 26, 28, 30}, // 21
	{28, 28, 30, 24}, // 22
	{30, 28, 30, 30}, // 23
	{30, 28, 30, 30}, // 24
	{26, 28, 30, 30}, // 25
	{28, 28, 28, 30}, // 26
	{30, 28, 30, 30}, // 27
	{30, 28, 30, 30}, // 28
	{30, 28, 30, 30}, // 29
	{30, 28, 30, 30}, // 30
	{30, 28, 30, 30}, // 31
	{30, 28, 30, 30}, // 32
	{30, 28, 30, 30}, // 33
	{30, 28, 30, 30}, // 34
	{30, 28, 30, 30}, // 35
	{30, 28, 30, 30}, // 36
	{30, 28, 30, 30}, // 37
	{30, 28, 30, 30}, // 38
	{30, 28, 30, 30}, // 39
	{30, 28, 30, 30}  // 40
};

static const uint8_t QR_TOTAL_RS_BLOCKS[40][4] = {
  //  L,  M,  Q,  H
	{ 1,  1,  1,  1}, //  1
	{ 1,  1,  1,  1}, //  2
	{ 1,  1,  2,  2}, //  3
	{ 1,  2,  2,  4}, //  4
	{ 1,  2,  4,  4}, //  5
	{ 2,  4,  4,  4}, //  6
	{ 2,  4,  6,  5}, //  7
	{ 2,  4,  6,  6}, //  8
	{ 2,  5,  8,  8}, //  9
	{ 4,  5,  8,  8}, // 10
	{ 4,  5,  8, 11}, // 11
	{ 4,  8, 10, 11}, // 12
	{ 4,  9, 12, 16}, // 13
	{ 4,  9, 16, 16}, // 14
	{ 6, 10, 12, 18}, // 15
	{ 6, 10, 17, 16}, // 16
	{ 6, 11, 16, 19}, // 17
	{ 6, 13, 18, 21}, // 18
	{ 7, 14, 21, 25}, // 19
	{ 8, 16, 20, 25}, // 20
	{ 8, 17, 23, 25}, // 21
	{ 9, 17, 23, 34}, // 22
	{ 9, 18, 25, 30}, // 23
	{10, 20, 27, 32}, // 24
	{12, 21, 29, 35}, // 25
	{12, 23, 34, 37}, // 26
	{12, 25, 34, 40}, // 27
	{13, 26, 35, 42}, // 28
	{14, 28, 38, 45}, // 29
	{15, 29, 40, 48}, // 30
	{16, 31, 43, 51}, // 31
	{17, 33, 45, 54}, // 32
	{18, 35, 48, 57}, // 33
	{19, 37, 51, 60}, // 34
	{19, 38, 53, 63}, // 35
	{20, 40, 56, 66}, // 36
	{21, 43, 59, 70}, // 37
	{22, 45, 62, 74}, // 38
	{24, 47, 65, 77}, // 39
	{25, 49, 68, 81}, // 40
};

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

size_t qrspec_get_available_bits(uint_fast8_t version) {
	size_t symbol_size = 17 + 4 * version;

	size_t finder_pattern = 8 * 8 * 3;
	size_t N = version > 1 ? (version / 7) + 2 : 0; // alignment_pattern_addr[version - 1][0];
	size_t alignment_pattern = version > 1 ? 5 * 5 * (N * N - 3) : 0;
	size_t timing_pattern = (symbol_size - 8 * 2 - (version > 1 ? 5 * (N - 2) : 0)) * 2;
	size_t version_info = version >= 7 ? 6 * 3 * 2 : 0;
	size_t format_info = 15 * 2 + 1;

	size_t function_bits = finder_pattern + alignment_pattern + timing_pattern + version_info + format_info;

	return symbol_size * symbol_size - function_bits;
}

uint_fast8_t qrspec_get_total_blocks(qr_version_t version, qr_errorlevel_t level) {
	return QR_TOTAL_RS_BLOCKS[version - QR_VERSION_1][level - QR_ERRORLEVEL_L];
}

uint_fast8_t qrspec_get_error_words_in_block(qr_version_t version, qr_errorlevel_t level) {
	return QR_ERROR_WORDS_IN_BLOCK[version - QR_VERSION_1][level - QR_ERRORLEVEL_L];
}
