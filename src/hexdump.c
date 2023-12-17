#include <stdio.h>

#ifndef HEXDUMP_PRINTF
#define HEXDUMP_PRINTF(...) fprintf(stderr, __VA_ARGS__)
#endif

int hexdump(const void *buf_, size_t len, unsigned long start_addr) {
	const int fold_size = 16;
	const int flag_print_chars = 1;
	const unsigned char *buf = (const unsigned char *)buf_;

	unsigned long i, j;
	unsigned long offset;
	unsigned long count;

	offset = start_addr % fold_size;
	count = (offset + len + fold_size - 1) / fold_size;

	for (i = 0; i < count; i++) {
		HEXDUMP_PRINTF("%08lx: ", start_addr);
		start_addr = (start_addr / fold_size) * fold_size;
		for (j = 0; j < fold_size; j++) {
			unsigned long idx = i * fold_size + j - offset;
			if (j % 8 == 0) HEXDUMP_PRINTF(" ");
			if (idx < len) {
				HEXDUMP_PRINTF("%02x ", buf[idx]);
			} else {
				HEXDUMP_PRINTF("   ");
			}
		}
		if (flag_print_chars) {
			HEXDUMP_PRINTF(" |");
			for (j = 0; j < fold_size; j++) {
				unsigned long idx = i * fold_size + j - offset;
				// if (j % 8 == 0) HEXDUMP_PRINTF(" ");
				if (idx < len) {
					HEXDUMP_PRINTF("%c", buf[idx] >= 0x20 && buf[idx] < 0x7fUL ? buf[idx] : '.');
				} else {
					HEXDUMP_PRINTF(" ");
				}
			}
			HEXDUMP_PRINTF("|");
		}
		HEXDUMP_PRINTF("\n");
		start_addr += fold_size;
	}
	return 0;
}
#undef HEXDUMP_PRINTF
