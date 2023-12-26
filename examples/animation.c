#include <stdio.h>

#include "qrean.h"

void dump(qrean_t *qrean) {
	static int image = 0;
	char fname[128];
	FILE *fp;

	snprintf(fname, sizeof(fname), "image-%04u.pgm", ++image);
	fp = fopen(fname, "wb");

	if (fp) {
		size_t len;

		size_t width = qrean_get_bitmap_width(qrean);
		size_t height = qrean_get_bitmap_height(qrean);

		char *buf = malloc(width * height);

		len = qrean_read_bitmap(qrean, buf, width * height, 8);

		fprintf(fp, "P5\n%zu %zu\n255\n", width, height);
		fwrite(buf, 1, len, fp);

		fclose(fp);

		free(buf);
	}
}

bit_t write_pixel(qrean_t *qrean, bitpos_t x, bitpos_t y, bitpos_t pos, bit_t v, void *opaque) {
	dump(qrean);

	return 0; // indicates let the library draw the pixel. Skip drawing otherwise.
}

int main() {
	qrean_t *qrean = new_qrean(QREAN_CODE_TYPE_EAN13);

	// This is needed to avoid auto detection also writes pixels internally
	qrean_set_qr_maskpattern(qrean, QR_MASKPATTERN_0);

	// Setup callbacks
	qrean_on_write_pixel(qrean, write_pixel, NULL);

	// Start drawing
	qrean_write_string(qrean, "123456789012", QREAN_DATA_TYPE_AUTO);

	// Flush for the final dot (NB; `write_pixel` is called everytime *before* the pixel draw)
	dump(qrean);
}
