#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>

// https://github.com/kikuchan/pngle
#include "pngle.h"

#include "hexdump.h"
#include "qrmatrix.h"

qrmatrix_t *qr;

void done(pngle_t *pngle)
{
	qrmatrix_set_format_info(qr, qrmatrix_read_format_info(qr));

	fprintf(stderr, "version; %d\n", qr->version);
	fprintf(stderr, "level; %d\n", qr->level);
	fprintf(stderr, "mask; %d\n", qr->mask);

	char buffer[1024];
	size_t len = qrmatrix_read_string(qr, buffer, sizeof(buffer));

	hexdump(buffer, len, 0);
	printf("RECV: %s\n", buffer);
}

void draw_pixel(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint8_t rgba[4])
{
	qrmatrix_write_pixel(qr, x, y, rgba[0] ? 0 : 1);
}

void init_screen(pngle_t *pngle, uint32_t x, uint32_t y)
{
	qrmatrix_set_version(qr, (x - 17) / 4);
}

// % qrencode -s 1 -m 0 -o - 'Hello' | ./decode
int main(int argc, char *argv[])
{
	char buf[1024];
	size_t remain = 0;
	int len;

	FILE *fp = stdin;

	qr = new_qrmatrix();

	pngle_t *pngle = pngle_new();

	pngle_set_init_callback(pngle, init_screen);
	pngle_set_draw_callback(pngle, draw_pixel);
	pngle_set_done_callback(pngle, done);

	while (!feof(fp)) {
		if (remain >= sizeof(buf)) {
			return -1;
		}

		len = fread(buf + remain, 1, sizeof(buf) - remain, fp);
		if (len <= 0) {
			break;
		}

		int fed = pngle_feed(pngle, buf, remain + len);
		if (fed < 0) {
			return -1;
		}

		remain = remain + len - fed;
		if (remain > 0) memmove(buf, buf + fed, remain);
	}

	return 0;
}
