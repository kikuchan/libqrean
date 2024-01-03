#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "image.h"
#include "qrean.h"
#include "qrspec.h"
#include "qrtypes.h"

#include "miniz.h"

#define BUFFER_SIZE (4096)

enum {
	SAVE_AS_DEFAULT,
	SAVE_AS_TXT,
	SAVE_AS_PPM,
	SAVE_AS_PNG,
};

const char *progname;
int usage(FILE *out)
{
	fprintf(out, "Usage: %s [OPTION]... [STRING]\n", progname);
	fprintf(out, "Generate QR/Barcode image\n");
	fprintf(out, "\n");
	fprintf(out, "  General options:\n");
	fprintf(out, "    -h                Display this help\n");
	fprintf(out, "    -o FILENAME       Image save as FILENAME\n");
	fprintf(out, "    -i FILENAME       Data read from FILENAME\n");
	fprintf(out, "    -f FORMAT         Output format, one of the following:\n");
	fprintf(out, "                          TXT    (default for tty)\n");
	fprintf(out, "                          PNG    (default for non tty)\n");
	fprintf(out, "                          PPM\n");
	fprintf(out, "    -t CODE           Output CODE, one of the following:\n");
	fprintf(out, "                          QR, mQR, rMQR, tQR\n");
	fprintf(out, "                          EAN13, EAN8, UPCA\n");
	fprintf(out, "                          CODE39, CODE93\n");
	fprintf(out, "                          ITF, NW7\n");
	fprintf(out, "    -p PADDING        Comma-separated PADDING for the code\n");
	fprintf(out, "\n");
	fprintf(out, "  QR family specific options:\n");
	fprintf(out, "    -v VERSION        Use VERSION, one of the following:\n");
	fprintf(out, "                          1 ... 40           (for QR)\n");
	fprintf(out, "                          M1 ... M4          (for mQR)\n");
	fprintf(out, "                          R7x43 ... R17x139  (for rMQR)\n");
	fprintf(out, "                            R{H}x{W}\n");
	fprintf(out, "                              H: 7, 9, 11, 13, 15, 17\n");
	fprintf(out, "                              W: 43, 59, 77, 99, 139\n");
	fprintf(out, "                          tQR                (for tQR)\n");
	fprintf(out, "    -m MASK           Use MASK pattern, one of the following:\n");
	fprintf(out, "                          0 ... 7            (for QR)\n");
	fprintf(out, "                          0 ... 4            (for mQR)\n");
	fprintf(out, "    -l LEVEL          Use ecc LEVEL, one of the following:\n");
	fprintf(out, "                          L, M, Q, H         (for QR)\n");
	fprintf(out, "                          L, M, Q            (for mQR)\n");
	fprintf(out, "                             M,    H         (for rMQR)\n");

	return 1;
}

void image_save_as_png(image_t *img, FILE *out)
{
	size_t len;
	void *png = tdefl_write_image_to_png_file_in_memory(img->buffer, img->width, img->height, 4, &len);
	if (png) {
		fwrite(png, 1, len, out);
		free(png);
	}
}

padding_t parse_padding(const char *src)
{
	padding_t padding = create_padding1(4);
	const char *p = src;
	unsigned long v;

	for (int i = 0; i < 4; i++, src = p) {
		v = strtoul(src, (char **)&p, 0);
		if (src == p) break;

		switch (i) {
		case 0:
			padding.t = padding.r = padding.b = padding.l = v;
			break;
		case 1:
			padding.r = padding.l = v;
			break;
		case 2:
			padding.b = v;
			break;
		case 3:
			padding.l = v;
			break;
		}

		if (p && *p && *p == ',') p++;
	}

	return padding;
}

int main(int argc, char *argv[])
{
	progname = argv[0];
	extern int optind;
	extern char *optarg;
	int ch;
	FILE *in = stdin;
	FILE *out = stdout;
	int save_as = SAVE_AS_DEFAULT;
	qrean_code_type_t code = QREAN_CODE_TYPE_QR;
	qr_version_t version = QR_VERSION_AUTO;
	qr_errorlevel_t level = QR_ERRORLEVEL_M;
	qr_maskpattern_t mask = QR_MASKPATTERN_AUTO;
	int scale = 4;
	padding_t padding = create_padding1(4);

	while ((ch = getopt(argc, argv, "hi:o:s:f:t:v:l:m:p:")) != -1) {
		int n;
		switch (ch) {
		case 'h':
			return usage(stdout);

		case 'i':
			in = fopen(optarg, "rb");
			if (in == NULL) {
				errx(1, "fopen failed. (%s: %s)", optarg, strerror(errno));
			}
			break;

		case 'o':
			out = fopen(optarg, "w+b");
			if (out == NULL) {
				errx(1, "fopen failed. (%s: %s)", optarg, strerror(errno));
			}
			break;

		case 's':
			scale = atoi(optarg);
			break;

		case 'f':
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

		case 't':
			if (!strcasecmp(optarg, "qr")) {
				code = QREAN_CODE_TYPE_QR;
			} else if (!strcasecmp(optarg, "mqr")) {
				code = QREAN_CODE_TYPE_MQR;
			} else if (!strcasecmp(optarg, "rmqr")) {
				code = QREAN_CODE_TYPE_RMQR;
			} else if (!strcasecmp(optarg, "tqr")) {
				code = QREAN_CODE_TYPE_TQR;
				version = QR_VERSION_TQR;
			} else if (!strcasecmp(optarg, "ean8")) {
				code = QREAN_CODE_TYPE_EAN8;
			} else if (!strcasecmp(optarg, "ean13")) {
				code = QREAN_CODE_TYPE_EAN13;
			} else if (!strcasecmp(optarg, "upca")) {
				code = QREAN_CODE_TYPE_UPCA;
			} else if (!strcasecmp(optarg, "code39")) {
				code = QREAN_CODE_TYPE_CODE39;
			} else if (!strcasecmp(optarg, "code93")) {
				code = QREAN_CODE_TYPE_CODE93;
			} else if (!strcasecmp(optarg, "itf")) {
				code = QREAN_CODE_TYPE_ITF;
			} else if (!strcasecmp(optarg, "nw7")) {
				code = QREAN_CODE_TYPE_NW7;
			} else {
				fprintf(stderr, "Unknown code '%s' is specified\n", optarg);
				return -1;
			}
			break;

		case 'v':
			version = qrspec_get_version_by_string(optarg);
			if (IS_QR(version)) {
				code = QREAN_CODE_TYPE_QR;
			} else if (IS_MQR(version)) {
				code = QREAN_CODE_TYPE_MQR;
			} else if (IS_RMQR(version)) {
				code = QREAN_CODE_TYPE_RMQR;
			} else if (IS_TQR(version)) {
				code = QREAN_CODE_TYPE_TQR;
			} else if (version == QR_VERSION_INVALID) {
				fprintf(stderr, "Unknown version '%s' is specified\n", optarg);
				return -1;
			}
			break;

		case 'l':
			switch (optarg[0]) {
			case 'l':
			case 'L':
				level = QR_ERRORLEVEL_L;
				break;
			case 'm':
			case 'M':
				level = QR_ERRORLEVEL_M;
				break;
			case 'q':
			case 'Q':
				level = QR_ERRORLEVEL_Q;
				break;
			case 'h':
			case 'H':
				level = QR_ERRORLEVEL_H;
				break;
			}
			break;

		case 'm':
			n = atoi(optarg);
			if (QR_MASKPATTERN_0 <= n && n <= QR_MASKPATTERN_7) {
				mask = (qr_maskpattern_t)n;
			}
			break;

		case 'p':
			padding = parse_padding(optarg);
			break;
		}
	}

	argc -= optind;
	argv += optind;

	char *data;
	size_t len;
	if (argc == 0) {
		if (isatty(fileno(in))) {
			return usage(stderr);
		}
		data = malloc(BUFFER_SIZE);
		len = fread(data, 1, BUFFER_SIZE, in);
	} else {
		data = argv[0];
		len = strlen(data);
	}

	if (!len) {
		fprintf(stderr, "No data\n");
		return -1;
	}

	qrean_t *qrean = new_qrean(code);
	if (!qrean) {
		fprintf(stderr, "Unknown code or version; %d\n", code);
		return -1;
	}

	qrean_set_bitmap_scale(qrean, scale);
	qrean_set_bitmap_padding(qrean, padding);
	if (QREAN_IS_TYPE_QRFAMILY(qrean)) {
		qrean_set_qr_version(qrean, version);
		qrean_set_qr_errorlevel(qrean, level);
		qrean_set_qr_maskpattern(qrean, mask);

		if (!qrean_check_qr_combination(qrean)) {
			fprintf(stderr, "Invalid combination of VERSION/LEVEL/MASK\n");
			return -1;
		}
	}

	if (save_as == SAVE_AS_DEFAULT) save_as = isatty(fileno(out)) ? SAVE_AS_TXT : SAVE_AS_PNG;

	size_t wrote = qrean_write_buffer(qrean, data, len, QREAN_DATA_TYPE_AUTO);
	if (!wrote) {
		fprintf(stderr, "Size exceed or mismatch\n");
		return -1;
	}

	size_t width = qrean_get_bitmap_width(qrean);
	size_t height = qrean_get_bitmap_height(qrean);
	size_t size = width * height * 4;
	image_t *img = new_image(width, height);

	qrean_read_bitmap(qrean, img->buffer, size, 32);

	switch (save_as) {
	case SAVE_AS_PNG:
		image_save_as_png(img, out);
		break;

	case SAVE_AS_PPM:
		image_save_as_ppm(img, out);
		break;

	default:
	case SAVE_AS_TXT:
		qrean_dump(qrean, out);
		break;
	}

	return 0;
}
