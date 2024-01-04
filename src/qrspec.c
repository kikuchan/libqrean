#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "qrspec.h"

static const uint8_t QR_ERROR_WORDS_IN_BLOCK[][4] = {
  // QR
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
	{30, 28, 30, 30}, // 40

#ifndef NO_MQR
  // mQR
  //  L,  M,  Q,  H
	{ 2,  0,  0,  0}, // M1
	{ 5,  6,  0,  0}, // M2
	{ 6,  8,  0,  0}, // M3
	{ 8, 10, 14,  0}, // M4
#endif

#ifndef NO_RMQR
  // rMQR
  //  L,  M,  Q,  H
	{ 0,  7,  0, 10}, // R7x43
	{ 0,  9,  0, 14}, // R7x59
	{ 0, 12,  0, 22}, // R7x77
	{ 0, 16,  0, 30}, // R7x99
	{ 0, 24,  0, 22}, // R7x139
	{ 0,  9,  0, 14}, // R9x43
	{ 0, 12,  0, 22}, // R9x59
	{ 0, 18,  0, 16}, // R9x77
	{ 0, 24,  0, 22}, // R9x99
	{ 0, 18,  0, 22}, // R9x139
	{ 0,  8,  0, 10}, // R11x27
	{ 0, 12,  0, 20}, // R11x43
	{ 0, 16,  0, 16}, // R11x59
	{ 0, 24,  0, 22}, // R11x77
	{ 0, 16,  0, 30}, // R11x99
	{ 0, 24,  0, 30}, // R11x139
	{ 0,  7,  0, 14}, // R13x27
	{ 0, 14,  0, 28}, // R13x43
	{ 0, 22,  0, 20}, // R13x59
	{ 0, 16,  0, 28}, // R13x77
	{ 0, 20,  0, 26}, // R13x99
	{ 0, 20,  0, 28}, // R13x139
	{ 0, 18,  0, 18}, // R15x43
	{ 0, 26,  0, 24}, // R15x59
	{ 0, 18,  0, 24}, // R15x77
	{ 0, 24,  0, 22}, // R15x99
	{ 0, 24,  0, 26}, // R15x139
	{ 0, 22,  0, 20}, // R17x43
	{ 0, 16,  0, 30}, // R17x59
	{ 0, 22,  0, 28}, // R17x77
	{ 0, 20,  0, 26}, // R17x99
	{ 0, 20,  0, 26}, // R17x139

	{11, 11, 11, 11}, // tqr
#endif
};

static const uint8_t QR_TOTAL_RS_BLOCKS[][4] = {
  // QR
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

#ifndef NO_MQR
  // mQR
  //  L,  M,  Q,  H
	{ 1,  0,  0,  0}, // M1
	{ 1,  1,  0,  0}, // M2
	{ 1,  1,  0,  0}, // M3
	{ 1,  1,  1,  0}, // M4
#endif

#ifndef NO_RMQR
  // rMQR
  //  L,  M,  Q,  H
	{ 0,  1,  0,  1}, // R7x43
	{ 0,  1,  0,  1}, // R7x59
	{ 0,  1,  0,  1}, // R7x77
	{ 0,  1,  0,  1}, // R7x99
	{ 0,  1,  0,  2}, // R7x139
	{ 0,  1,  0,  1}, // R9x43
	{ 0,  1,  0,  1}, // R9x59
	{ 0,  1,  0,  2}, // R9x77
	{ 0,  1,  0,  2}, // R9x99
	{ 0,  2,  0,  3}, // R9x139
	{ 0,  1,  0,  1}, // R11x27
	{ 0,  1,  0,  1}, // R11x43
	{ 0,  1,  0,  2}, // R11x59
	{ 0,  1,  0,  2}, // R11x77
	{ 0,  2,  0,  2}, // R11x99
	{ 0,  2,  0,  3}, // R11x139
	{ 0,  1,  0,  1}, // R13x27
	{ 0,  1,  0,  1}, // R13x43
	{ 0,  1,  0,  2}, // R13x59
	{ 0,  2,  0,  2}, // R13x77
	{ 0,  2,  0,  3}, // R13x99
	{ 0,  3,  0,  4}, // R13x139
	{ 0,  1,  0,  2}, // R15x43
	{ 0,  1,  0,  2}, // R15x59
	{ 0,  2,  0,  3}, // R15x77
	{ 0,  2,  0,  4}, // R15x99
	{ 0,  3,  0,  5}, // R15x139
	{ 0,  1,  0,  2}, // R17x43
	{ 0,  2,  0,  2}, // R17x59
	{ 0,  2,  0,  3}, // R17x77
	{ 0,  3,  0,  4}, // R17x99
	{ 0,  4,  0,  6}, // R17x139
#endif
	{ 1,  1,  1,  1}, // tqr
};

#ifndef NO_RMQR
static const uint8_t RMQR_VERSION_TYPES[] = {
	0x01,
	0x02,
	0x03,
	0x04,
	0x05,
	0x11,
	0x12,
	0x13,
	0x14,
	0x15,
	0x20,
	0x21,
	0x22,
	0x23,
	0x24,
	0x25,
	0x30,
	0x31,
	0x32,
	0x33,
	0x34,
	0x35,
	0x41,
	0x42,
	0x43,
	0x44,
	0x45,
	0x51,
	0x52,
	0x53,
	0x54,
	0x55,
};

#define RMQR_SYMBOL_X_INDEX(version) (RMQR_VERSION_TYPES[(version)-QR_VERSION_R7x43] & 0xF)
#define RMQR_SYMBOL_Y_INDEX(version) (RMQR_VERSION_TYPES[(version)-QR_VERSION_R7x43] >> 4)

static const uint8_t RMQR_SYMBOL_WIDTH[6] = {
	27,
	43,
	59,
	77,
	99,
	139,
};

static const uint8_t RMQR_SYMBOL_HEIGHT[6] = { 7, 9, 11, 13, 15, 17 };

static const uint8_t RMQR_ALIGNMENT_ADDRS[6][4] = {
	{ 0,  0,  0,   0}, // 27
	{21,  0,  0,   0}, // 43
	{19, 39,  0,   0}, // 59
	{25, 51,  0,   0}, // 77
	{23, 49, 75,   0}, // 99
	{27, 55, 83, 111}, // 139
};
static const uint8_t RMQR_ALIGNMENT_NUMS[6] = { 0, 1, 2, 2, 3, 4 };
#endif

static const uint8_t DATA_LENGTH_TABLE[][4] = {
  // QR
  //  N, AL, 8B, KNJ
	{10,  9,  8,  8}, // QR < 10
	{12, 11, 16, 10}, // QR < 27
	{14, 13, 16, 12}, // QR >= 27

  // mQR
  //  N, AL, 8B, KNJ
	{ 3,  0,  0,  0}, // mQR  M1
	{ 4,  3,  0,  0}, // mQR  M2
	{ 5,  4,  4,  3}, // mQR  M3
	{ 6,  5,  5,  4}, // mQR  M4

  // rMQR
  //  N, AL, 8B, KNJ
	{ 4,  3,  3,  2}, // R7x43
	{ 5,  5,  4,  3}, // R7x59
	{ 6,  5,  5,  4}, // R7x77
	{ 7,  6,  5,  5}, // R7x99
	{ 7,  6,  6,  5}, // R7x139

	{ 5,  5,  4,  3}, // R9x43
	{ 6,  5,  5,  4}, // R9x59
	{ 7,  6,  5,  5}, // R9x77
	{ 7,  6,  6,  5}, // R9x99
	{ 8,  7,  6,  6}, // R9x139

	{ 4,  4,  3,  2}, // R11x27
	{ 6,  5,  5,  4}, // R11x43
	{ 7,  6,  5,  5}, // R11x59
	{ 7,  6,  6,  5}, // R11x77
	{ 8,  7,  6,  6}, // R11x99
	{ 8,  7,  7,  6}, // R11x139

	{ 5,  5,  4,  3}, // R13x27
	{ 6,  6,  5,  5}, // R13x43
	{ 7,  6,  6,  5}, // R13x59
	{ 7,  7,  6,  6}, // R13x77
	{ 8,  7,  7,  6}, // R13x99
	{ 8,  8,  7,  7}, // R13x139

	{ 7,  6,  6,  5}, // R15x43
	{ 7,  7,  6,  5}, // R15x59
	{ 8,  7,  7,  6}, // R15x77
	{ 8,  7,  7,  6}, // R15x99
	{ 9,  8,  7,  7}, // R15x139

	{ 7,  6,  6,  5}, // R17x43
	{ 8,  7,  6,  6}, // R17x59
	{ 8,  7,  7,  6}, // R17x77
	{ 8,  8,  7,  6}, // R17x99
	{ 9,  8,  8,  7}, // R17x139
};

uint_fast8_t qrspec_get_data_bitlength_for(qr_version_t version, int mode)
{
	if (version < QR_VERSION_10) return DATA_LENGTH_TABLE[0][mode];
	if (version < QR_VERSION_27) return DATA_LENGTH_TABLE[1][mode];
	if (version < QR_VERSION_40) return DATA_LENGTH_TABLE[2][mode];

#ifndef NO_MQR
	if (IS_MQR(version)) return DATA_LENGTH_TABLE[3 + version - QR_VERSION_M1][mode];
#endif
#ifndef NO_RMQR
	if (IS_RMQR(version)) return DATA_LENGTH_TABLE[7 + version - QR_VERSION_R7x43][mode];
#endif

	return 0;
}

uint_fast8_t qrspec_get_symbol_width(qr_version_t version)
{
	if (version == QR_VERSION_AUTO) version = QR_VERSION_40;
	if (IS_QR(version)) {
		return 21 + (version - QR_VERSION_1) * 4;
#ifndef NO_MQR
	} else if (IS_MQR(version)) {
		return 11 + (version - QR_VERSION_M1) * 2;
#endif
#ifndef NO_RMQR
	} else if (IS_RMQR(version)) {
		return RMQR_SYMBOL_WIDTH[RMQR_SYMBOL_X_INDEX(version)];
#endif
#ifndef NO_TQR
	} else if (IS_TQR(version)) {
		return 23;
#endif
	}

	qrean_error("Invalid version is specified");
	return 0;
}

uint_fast8_t qrspec_get_symbol_height(qr_version_t version)
{
	if (version == QR_VERSION_AUTO) version = QR_VERSION_40;
	if (IS_QR(version)) {
		return 21 + (version - QR_VERSION_1) * 4;
#ifndef NO_MQR
	} else if (IS_MQR(version)) {
		return 11 + (version - QR_VERSION_M1) * 2;
#endif
#ifndef NO_RMQR
	} else if (IS_RMQR(version)) {
		return RMQR_SYMBOL_HEIGHT[RMQR_SYMBOL_Y_INDEX(version)];
#endif
#ifndef NO_TQR
	} else if (IS_TQR(version)) {
		return 23;
#endif
	}

	qrean_error("Invalid version is specified");
	return 0;
}

uint_fast8_t qrspec_get_alignment_num(qr_version_t version)
{
	if (version <= QR_VERSION_AUTO) {
		qrean_error("Invalid version is specified");
		return 0;
	}

	if (QR_VERSION_2 <= version && version <= QR_VERSION_40) {
		int N = version / 7 + 2;
		return N * N - 3;
	}

#ifndef NO_RMQR
	if (IS_RMQR(version)) {
		return RMQR_ALIGNMENT_NUMS[RMQR_SYMBOL_X_INDEX(version)] * 2;
	}
#endif

	// others: version 1, M1 - M4
	return 0;
}

uint_fast8_t qrspec_get_alignment_steps(qr_version_t version, uint_fast8_t step)
{
	if (version <= QR_VERSION_1 || IS_MQR(version)) return 0;
#ifndef NO_RMQR
	if (IS_RMQR(version)) {
		return RMQR_ALIGNMENT_ADDRS[RMQR_SYMBOL_X_INDEX(version)][step];
	}
#endif

	uint_fast8_t N = version / 7 + 2;
	uint_fast8_t r = ((((version + 1) * 8 / (N - 1)) + 3) / 4) * 2 * (N - step - 1);
	uint_fast8_t v4 = version * 4;

	if (step >= N) return 0;

	return v4 < r ? 6 : v4 - r + 10;
}

uint_fast8_t qrspec_get_alignment_position_x(qr_version_t version, uint_fast8_t idx)
{
	if (IS_QR(version)) {
		int N = version / 7 + 2;
		int xidx = (idx + 1) < (N - 1) * 1 ? (idx + 1) % N : (idx + 2) < (N - 1) * N ? (idx + 2) % N : (idx + 3) % N;
		return qrspec_get_alignment_steps(version, xidx);
	}
#ifndef NO_RMQR
	if (IS_RMQR(version)) {
		return qrspec_get_alignment_steps(version, idx / 2);
	}
#endif
	return 0;
}

uint_fast8_t qrspec_get_alignment_position_y(qr_version_t version, uint_fast8_t idx)
{
	if (IS_QR(version)) {
		int N = version / 7 + 2;
		int yidx = (idx + 1) < (N - 1) * 1 ? (idx + 1) / N : (idx + 2) < (N - 1) * N ? (idx + 2) / N : (idx + 3) / N;
		return qrspec_get_alignment_steps(version, yidx);
	}
#ifndef NO_RMQR
	if (IS_RMQR(version)) {
		return idx % 2 == 0 ? 1 : qrspec_get_symbol_height(version) - 2;
	}
#endif
	return 0;
}

size_t qrspec_get_available_bits(qr_version_t version)
{
	size_t symbol_width = qrspec_get_symbol_width(version);
	size_t symbol_height = qrspec_get_symbol_height(version);

	if (IS_QR(version)) {
		size_t finder_pattern = 8 * 8 * 3;
		size_t N = version > 1 ? (version / 7) + 2 : 0; // alignment_pattern_addr[version - 1][0];
		size_t alignment_pattern = version > 1 ? 5 * 5 * (N * N - 3) : 0;
		size_t timing_pattern = (symbol_width - 8 * 2 - (version > 1 ? 5 * (N - 2) : 0)) * 2;
		size_t version_info = version >= 7 ? 6 * 3 * 2 : 0;
		size_t format_info = 15 * 2 + 1;

		size_t function_bits = finder_pattern + alignment_pattern + timing_pattern + version_info + format_info;

		return symbol_width * symbol_height - function_bits;
#ifndef NO_MQR
	} else if (IS_MQR(version)) {
		size_t finder_pattern = 8 * 8 * 1;
		size_t timing_pattern = (symbol_width - 8) + (symbol_height - 8);
		size_t format_info = 15;

		size_t function_bits = finder_pattern + timing_pattern + format_info;

		return symbol_width * symbol_height - function_bits;
#endif
#ifndef NO_RMQR
	} else if (IS_RMQR(version)) {
		size_t finder_pattern = 8 * 7 + (symbol_height > 7 ? 8 : 0);
		size_t finder_sub_pattern = 5 * 5 * 1;
		size_t format_info = 18 * 2;
		size_t corner_finder_pattern = (symbol_height >= 8 + 3 ? 6 * 2 : symbol_height > 8 ? 3 + 6 : 5);

		size_t num_aligns_x = RMQR_ALIGNMENT_NUMS[RMQR_SYMBOL_X_INDEX(version)];
		size_t timing_pattern_t = symbol_width - 8 - 3 * num_aligns_x - 3;
		size_t timing_pattern_b = symbol_width - (symbol_height > 8 ? 3 : 8) - 3 * num_aligns_x - 5;
		size_t timing_pattern_r = symbol_height > 8 ? symbol_height - 3 - 5 : 0;
		size_t timing_pattern_l = symbol_height > 8 + 3 ? symbol_height - 8 - 3 : 0;
		size_t timing_pattern_v = symbol_height > 6 ? (symbol_height - 3 - 3) * num_aligns_x : 0;
		size_t timing_pattern = timing_pattern_t + timing_pattern_b + timing_pattern_r + timing_pattern_l + timing_pattern_v;
		size_t alignment_pattern = (3 * 3) * 2 * num_aligns_x;

		size_t function_bits
			= finder_pattern + finder_sub_pattern + corner_finder_pattern + timing_pattern + alignment_pattern + format_info;

		return symbol_width * symbol_height - function_bits;
#endif
#ifndef NO_TQR
	} else if (IS_TQR(version)) {
		return 19 * 19 - 3 * 2 - 8 * 8 * 3 - 3;
#endif
	}

	qrean_error("Invalid version is specified");
	return 0;
}

uint_fast8_t qrspec_get_total_blocks(qr_version_t version, qr_errorlevel_t level)
{
	return QR_TOTAL_RS_BLOCKS[version - QR_VERSION_1][level - QR_ERRORLEVEL_L];
}

uint_fast8_t qrspec_get_error_words_in_block(qr_version_t version, qr_errorlevel_t level)
{
	return QR_ERROR_WORDS_IN_BLOCK[version - QR_VERSION_1][level - QR_ERRORLEVEL_L];
}

// XXX: must be the same order of the enum
static const char *qr_version_string[] = {
	"AUTO",

	// QR
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	"10",
	"11",
	"12",
	"13",
	"14",
	"15",
	"16",
	"17",
	"18",
	"19",
	"20",
	"21",
	"22",
	"23",
	"24",
	"25",
	"26",
	"27",
	"28",
	"29",
	"30",
	"31",
	"32",
	"33",
	"34",
	"35",
	"36",
	"37",
	"38",
	"39",
	"40",

	// mQR
	"M1",
	"M2",
	"M3",
	"M4",

	// rMQR
	"R7x43",
	"R7x59",
	"R7x77",
	"R7x99",
	"R7x139",

	"R9x43",
	"R9x59",
	"R9x77",
	"R9x99",
	"R9x139",

	"R11x27",
	"R11x43",
	"R11x59",
	"R11x77",
	"R11x99",
	"R11x139",

	"R13x27",
	"R13x43",
	"R13x59",
	"R13x77",
	"R13x99",
	"R13x139",

	"R15x43",
	"R15x59",
	"R15x77",
	"R15x99",
	"R15x139",

	"R17x43",
	"R17x59",
	"R17x77",
	"R17x99",
	"R17x139",

	"tQR",
};

const char *qrspec_get_version_string(qr_version_t version)
{
	return qr_version_string[version];
}

qr_version_t qrspec_get_version_by_string(const char *version)
{
	for (size_t i = 0; i < sizeof(qr_version_string) / sizeof(qr_version_string[0]); i++) {
		if (!strcasecmp(qr_version_string[i], version)) return (qr_version_t)i;
	}
	return QR_VERSION_INVALID;
}

int qrspec_is_valid_combination(qr_version_t version, qr_errorlevel_t level, qr_maskpattern_t mask)
{
	if (version <= QR_VERSION_INVALID || mask <= QR_MASKPATTERN_INVALID) return 0;
	if (!qrspec_get_error_words_in_block(version, level)) return 0;

	if (mask == QR_MASKPATTERN_AUTO) return 1;
	if (IS_QR(version) && QR_MASKPATTERN_0 <= mask && mask <= QR_MASKPATTERN_7) return 1;
	if (IS_MQR(version) && QR_MASKPATTERN_0 <= mask && mask <= QR_MASKPATTERN_3) return 1;
	if (IS_RMQR(version) && QR_MASKPATTERN_0 <= mask && mask <= QR_MASKPATTERN_0) return 1;
	if (IS_TQR(version) && QR_MASKPATTERN_0 <= mask && mask <= QR_MASKPATTERN_0) return 1;
	return 0;
}
