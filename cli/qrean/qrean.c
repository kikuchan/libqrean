#include "qrdata.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef __WIN32
#include <windows.h>
#endif

#include "image.h"
#include "qrean.h"
#include "qrspec.h"

#include "miniz.h"

#define BUFFER_SIZE (4096)

enum {
	SAVE_AS_DEFAULT,
	SAVE_AS_TXT,
	SAVE_AS_PPM,
	SAVE_AS_PNG,
};

const char *progname;

void print_version(FILE *out)
{
	fprintf(out, "qrean version %s\n", qrean_version());
	fprintf(out, "Copyright (c) 2023-2024 kikuchan\n");
}

int usage(FILE *out)
{
	print_version(out);
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
	fprintf(out, "    -s SCALE          Bitmap SCALE (default: 4)\n");
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
	fprintf(out, "    -N                Use Numeric mode only\n");
	fprintf(out, "    -A                Use Alpha numeric mode only\n");
	fprintf(out, "    -K                Use Kanji mode only\n");
	fprintf(out, "    -8                Use 8bit mode only\n");
	fprintf(out, "\n");
	fprintf(out, "  ECI options:\n");
	fprintf(out, "    -E ECI            Set ECI code\n");
	fprintf(out, "    -S                Set ECI code to ShiftJIS (-E 20)\n");
	fprintf(out, "    -U                Set ECI code to UTF-8 (-E 26)\n");

	return 1;
}

void image_save_as_png(image_t *img, FILE *out)
{
	size_t len;

	uint8_t *buf = (uint8_t *)malloc(img->width * img->height * 4);
	if (!buf) return;

	for (int y = 0; y < img->height; y++) {
		for (int x = 0; x < img->width; x++) {
			image_pixel_t pix = image_read_pixel(img, POINT(x, y));
			buf[(y * img->width + x) * 4 + 0] = PIXEL_GET_R(pix);
			buf[(y * img->width + x) * 4 + 1] = PIXEL_GET_G(pix);
			buf[(y * img->width + x) * 4 + 2] = PIXEL_GET_B(pix);
			buf[(y * img->width + x) * 4 + 3] = 255;
		}
	}

	void *png = tdefl_write_image_to_png_file_in_memory(buf, img->width, img->height, 4, &len);
	if (png) {
		fwrite(png, 1, len, out);
		free(png);
	}

	free(buf);
}

padding_t *parse_padding(const char *src, padding_t *padding)
{
	const char *p = src;
	unsigned long v;

	for (int i = 0; i < 4; i++, src = p) {
		v = strtoul(src, (char **)&p, 0);
		if (src == p) break;

		switch (i) {
		case 0:
			padding->t = padding->r = padding->b = padding->l = v;
			break;
		case 1:
			padding->r = padding->l = v;
			break;
		case 2:
			padding->b = v;
			break;
		case 3:
			padding->l = v;
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
	padding_t default_padding = create_padding1(4), *padding = NULL;
	qrean_data_type_t data_type = QREAN_DATA_TYPE_AUTO;
	int eci_code = QR_ECI_CODE_LATIN1;

	while ((ch = getopt(argc, argv, "hVi:o:s:f:t:v:l:m:p:8KANUSE:")) != -1) {
		int n;
		switch (ch) {
		case 'h':
			return usage(stdout);

		case 'V':
			print_version(stdout);
			return 0;

		case 'i':
			in = fopen(optarg, "rb");
			if (in == NULL) {
				fprintf(stderr, "fopen failed. (%s: %s)", optarg, strerror(errno));
				return 1;
			}
			break;

		case 'o':
			out = fopen(optarg, "w+b");
			if (out == NULL) {
				fprintf(stderr, "fopen failed. (%s: %s)", optarg, strerror(errno));
				return 1;
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
				fprintf(stderr, "Unknown safe format\n");
				return 1;
			}
			break;

		case 't':
			code = qrean_get_code_type_by_string(optarg);
			if (code == QREAN_CODE_TYPE_INVALID) {
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
			padding = parse_padding(optarg, &default_padding);
			break;

		case '8':
			data_type = QREAN_DATA_TYPE_8BIT;
			break;
		case 'K':
			data_type = QREAN_DATA_TYPE_KANJI;
			break;
		case 'A':
			data_type = QREAN_DATA_TYPE_ALNUM;
			break;
		case 'N':
			data_type = QREAN_DATA_TYPE_NUMERIC;
			break;

		case 'U':
			eci_code = QR_ECI_CODE_UTF8;
			break;

		case 'S':
			eci_code = QR_ECI_CODE_SJIS;
			break;

		case 'E':
			eci_code = atoi(optarg);
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

	qrean_t qrean = create_qrean(code);
	if (!qrean_is_valid(&qrean)) {
		fprintf(stderr, "Unknown code or version; %d\n", code);
		return -1;
	}

	qrean_set_bitmap_scale(&qrean, scale);
	if (padding) qrean_set_bitmap_padding(&qrean, *padding);
	if (QREAN_IS_TYPE_QRFAMILY(&qrean)) {
		qrean_set_qr_version(&qrean, version);
		qrean_set_qr_errorlevel(&qrean, level);
		qrean_set_qr_maskpattern(&qrean, mask);

		if (!qrean_check_qr_combination(&qrean)) {
			fprintf(stderr, "Invalid combination of VERSION/LEVEL/MASK\n");
			return -1;
		}

		qrean_set_eci_code(&qrean, eci_code);
	}

	if (save_as == SAVE_AS_DEFAULT) save_as = isatty(fileno(out)) ? SAVE_AS_TXT : SAVE_AS_PNG;

	size_t wrote = qrean_write_buffer(&qrean, data, len, data_type);
	if (!wrote) {
		fprintf(stderr, "Size exceed or mismatch\n");
		return -1;
	}

	size_t width = qrean_get_bitmap_width(&qrean);
	size_t height = qrean_get_bitmap_height(&qrean);
	size_t size = width * height * 4;

	CREATE_IMAGE(img, width, height);

	qrean_read_bitmap(&qrean, img.buffer, size, 32);

	switch (save_as) {
	case SAVE_AS_PNG:
		image_save_as_png(&img, out);
		break;

	case SAVE_AS_PPM:
		image_save_as_ppm(&img, out);
		break;

	default:
	case SAVE_AS_TXT:
#ifdef __WIN32
	{
		unsigned int cp = GetConsoleOutputCP();
		SetConsoleOutputCP(65001);
#endif
		qrean_dump(&qrean, out);
#ifdef __WIN32
		SetConsoleOutputCP(cp);
	}
#endif
		break;
	}

	DESTROY_IMAGE(img);

	return 0;
}
