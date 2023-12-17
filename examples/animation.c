#include <stdio.h>

#include "qrmatrix.h"

void dump(qrmatrix_t *qr) {
	static int image = 0;
	char fname[128];
	FILE *fp;

	snprintf(fname, sizeof(fname), "image-%04u.pgm", ++image);
	fp = fopen(fname, "wb");

	if (fp) {
		char buf[1024];
		size_t len;

		len = qrmatrix_read_bitmap(qr, buf, sizeof(buf), 8);

		fprintf(fp, "P5\n%u %u\n255\n", qr->width, qr->height);
		fwrite(buf, 1, len, fp);

		fclose(fp);
	}
}

bit_t write_pixel(qrmatrix_t *qr, bitpos_t x, bitpos_t y, bitpos_t pos, bit_t v, void *opaque) {
	dump(qr);

	return 0; // indicates let the library draw the pixel. Skip drawing otherwise.
}

int main() {
	qrmatrix_t *qr = new_qrmatrix();

	// This is needed to avoid auto detection also writes pixels internally
	qrmatrix_set_maskpattern(qr, QR_MASKPATTERN_0);

	// Setup callbacks
	qrmatrix_on_write_pixel(qr, write_pixel, NULL);

	// Start drawing
	qrmatrix_write_string(qr, "Hello, world");

	// Flush for the final dot (NB; `write_pixel` is called everytime *before* the pixel draw)
	dump(qr);
}
