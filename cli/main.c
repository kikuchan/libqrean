#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "image.h"
#include "qrmatrix.h"
#include "qrspec.h"
#include "qrtypes.h"

#include "miniz.h"

enum {
	SAVE_AS_DEFAULT,
	SAVE_AS_TXT,
	SAVE_AS_PPM,
	SAVE_AS_PNG,
};

const char *progname;
int usage(FILE *out) {
	fprintf(out, "Usage: %s [options] <data>\n", progname);
	fprintf(out, "  -o <filename>: output to the file\n");
	fprintf(out, "  -t <type>: TXT, PNG, or PPM (default: TXT on tty, PNG otherwise)\n");
	fprintf(out, "  -v <QR version / Barcode type>:\n");
	fprintf(out, "       QR: 1, 2, ... 40\n");
	fprintf(out, "      mQR: M1, M2, ... M4\n");
	fprintf(out, "     rMQR: R7x43, ... R17x139\n");
	fprintf(out, "  -l {LMQH}: error level for QR family\n");
	fprintf(out, "  -h: This help\n");
	return 1;
}

void image_save_as_png(image_t *img, FILE *out) {
	size_t len;
	void *png = tdefl_write_image_to_png_file_in_memory(img->buffer, img->width, img->height, 4, &len);
	if (png) {
		fwrite(png, 1, len, out);
		free(png);
	}
}

int main(int argc, char *argv[]) {
	progname = argv[0];
	extern int optind;
	extern char *optarg;
	int ch;
	FILE *out = stdout;
	int save_as = SAVE_AS_DEFAULT;
	qr_version_t version = QR_VERSION_AUTO;

	qrmatrix_t *qr = new_qrmatrix();

	while ((ch = getopt(argc, argv, "ho:s:t:v:l:")) != -1) {
		int n = atoi(optarg);
		switch (ch) {
		case 'h':
			return usage(stdout);

		case 'o':
			out = fopen(optarg, "w+b");
			if (out == NULL) {
				errx(1, "fopen failed. (%s: %s)", optarg, strerror(errno));
			}
			break;

		case 's':
			qrmatrix_set_bitmap_scale(qr, atoi(optarg));
			break;

		case 't':
			if (!strcasecmp(optarg, "png")) {
				save_as = SAVE_AS_PNG;
			} else if (!strcasecmp(optarg, "ppm")) {
				save_as = SAVE_AS_PPM;
			} else if (!strcasecmp(optarg, "txt")) {
				save_as = SAVE_AS_TXT;
			} else {
				errx(1, "Unknown safe format\n");
			}
			break;

		case 'v':
			version = qrspec_get_version_by_string(optarg);
			if (version == QR_VERSION_INVALID) {
				errx(1, "Unknown version specified\n");
			}
			qrmatrix_set_version(qr, version);
			break;

		case 'l':
			switch (optarg[0]) {
			case 'L':
				qrmatrix_set_errorlevel(qr, QR_ERRORLEVEL_L);
				break;
			case 'M':
				qrmatrix_set_errorlevel(qr, QR_ERRORLEVEL_M);
				break;
			case 'Q':
				qrmatrix_set_errorlevel(qr, QR_ERRORLEVEL_Q);
				break;
			case 'H':
				qrmatrix_set_errorlevel(qr, QR_ERRORLEVEL_H);
				break;
			}
			break;

		case 'm':
			if (QR_MASKPATTERN_0 <= n && n <= QR_MASKPATTERN_7) {
				qrmatrix_set_maskpattern(qr, (qr_maskpattern_t)n);
			}
			break;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc == 0) {
		return usage(stderr);
	}

	const char *data = argv[0];
	if (save_as == SAVE_AS_DEFAULT) save_as = isatty(fileno(out)) ? SAVE_AS_TXT : SAVE_AS_PNG;

	qrmatrix_write_string(qr, data);

	int pixel_size = qrmatrix_get_bitmap_scale(qr);
	size_t size = qr->width * pixel_size * qr->height * pixel_size * 4;
	image_t *img = new_image(qr->width * pixel_size, qr->height * pixel_size);

	qrmatrix_set_bitmap_scale(qr, pixel_size);
	qrmatrix_read_bitmap(qr, img->buffer, size, 32);

	switch (save_as) {
	case SAVE_AS_PNG:
		image_save_as_png(img, out);
		break;

	case SAVE_AS_PPM:
		image_save_as_ppm(img, out);
		break;

	default:
	case SAVE_AS_TXT:
		qrmatrix_dump(qr, out);
		break;
	}

	return 0;
}
