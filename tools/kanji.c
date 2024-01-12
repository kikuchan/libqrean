#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

static int sjis2index(int code) {
	int hi = (code >> 8) & 0xff;
	int lo = (code >> 0) & 0xff;

	if (hi >= 0x81 && hi <= 0x9f) {
		hi -= 0x81;
	} else if (hi >= 0xe0 && hi <= 0xfc) {
		hi -= 0xc1;
	} else {
		return -1;
	}
	if (lo >= 0x40 && lo <= 0xfc) {
		lo -= 0x40;
	} else {
		return -1;
	}

	return hi * 0xc0 + lo;
}

int main()
{
	FILE *fp = fopen("CP932.TXT", "rt");
	if (!fp) return -1;

	char buf[1024];

#define MAX_IDX (sjis2index(0xfcf4))

	uint16_t *tbl = calloc(MAX_IDX + 1, sizeof(uint16_t));
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		char *ptr = buf;
		int code = strtoul(ptr, &ptr, 0);
		int uni = strtoul(ptr, &ptr, 0);

		if (code < 0x100) continue;

		int idx = sjis2index(code);
		if (idx < 0) {
			fprintf(stderr, "ERROR\n");
			continue;
		}
		if (idx > MAX_IDX) {
			fprintf(stderr, "ERROR\n");
			continue;
		}

		tbl[idx] = uni;
	}

	printf("#include <stdint.h>\n\n");
	printf("// clang-format off\n");
	printf("static uint16_t cp932_to_unicode_tbl[] = {");
	for (int i = 0; i <= MAX_IDX; i++) {
		if (i % 16 == 0) printf("\n   ");
		printf(" 0x%04x,", tbl[i]);
	}
	printf("\n};\n");
	printf("// clang-format on\n");
}
