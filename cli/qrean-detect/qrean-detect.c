#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "miniz.h"
#include "pngle.h"

#include "debug.h"
#include "detector.h"
#include "qrean.h"
#include "qrpayload.h"
#include "qrspec.h"
#include "runlength.h"

#include "image.h"

image_t *img;

image_t *detected;
FILE *debug_out;
FILE *out;
int flag_debug = 0;
int flag_verbose = 0;
int flag_bench = 0;
double gamma_value = 1.0;
int eci_code = QR_ECI_CODE_LATIN1;

uint32_t W = 0;
uint32_t H = 0;

bit_t write_image_pixel(qrean_t *qrean, bitpos_t x, bitpos_t y, bitpos_t pos, bit_t value, void *opaque)
{
	qrean_detector_perspective_t *warp = (qrean_detector_perspective_t *)opaque;
	image_draw_pixel(detected, image_point_transform(POINT(x, y), warp->h), value ? PIXEL(255, 0, 0) : PIXEL(0, 255, 0));
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

static void on_found(qrean_detector_perspective_t *warp, void *opaque)
{
	char buffer[1024];

	if (detected) {
		if (QREAN_IS_TYPE_QRFAMILY(warp->qrean)) {
			image_point_t points[] = {
				image_point_transform(POINT(-0.5, -0.5), warp->h),
				image_point_transform(POINT(warp->qrean->canvas.symbol_width - 0.5, -0.5), warp->h),
				image_point_transform(POINT(warp->qrean->canvas.symbol_width - 0.5, warp->qrean->canvas.symbol_height - 0.5), warp->h),
				image_point_transform(POINT(-0.5, warp->qrean->canvas.symbol_height - 0.5), warp->h),
			};
			image_draw_polygon(detected, 4, points, PIXEL(0, 255, 0), 1);

			// XXX: backup
			bit_t (*backup)(qrean_t *qrean, bitpos_t x, bitpos_t y, bitpos_t pos, bit_t v, void *opaque) = warp->qrean->canvas.write_pixel;
			qrean_on_write_pixel(warp->qrean, write_image_pixel, warp);

			qrpayload_t payload = create_qrpayload(warp->qrean->qr.version, warp->qrean->qr.level);
			qrean_read_qr_payload(warp->qrean, &payload);
			qrean_write_frame(warp->qrean);
			qrean_write_qr_payload(warp->qrean, &payload);

			warp->qrean->canvas.write_pixel = backup;

			qrpayload_dump(&payload, stderr);
		} else {
			// XXX: backup
			bit_t (*backup)(qrean_t *qrean, bitpos_t x, bitpos_t y, bitpos_t pos, bit_t v, void *opaque) = warp->qrean->canvas.write_pixel;
			qrean_on_write_pixel(warp->qrean, write_image_pixel, warp);

			size_t len = qrean_read_string(warp->qrean, buffer, sizeof(buffer));
			qrean_write_string(warp->qrean, buffer, len);

			warp->qrean->canvas.write_pixel = backup;
		}
	}

	if (eci_code >= 0) qrean_set_eci_code(warp->qrean, eci_code);
	qrean_read_string(warp->qrean, buffer, sizeof(buffer));

	if (flag_verbose) {
		fprintf(out, "%s: ", qrean_get_code_type_string(warp->qrean->code->type));
	}
	fprintf(out, "%s\n", buffer);
}

void decode(image_t* img)
{
}

void done(pngle_t *pngle)
{
	image_t *mono = image_clone(img);

	image_digitize(mono, img, gamma_value);
	// image_morphology_open(mono);
	// image_morphology_close(mono);

	int found = 0;
	int n = flag_bench ? 10 : 1;

	struct timeval tv, tv2;
	gettimeofday(&tv, NULL);
	for (int i = 0; i < n; i++) {
		int num_candidates;
		qrean_detector_qr_finder_candidate_t *candidates = qrean_detector_scan_qr_finder_pattern(mono, &num_candidates);

		if (flag_debug) {
			// detected = image_clone(img);
			detected = image_clone(mono);
			for (int i = 0; i < num_candidates; i++) {
				image_draw_filled_ellipse(detected, candidates[i].center, 5, 5, PIXEL(255, 0, 0));
				image_draw_extent(detected, candidates[i].extent, PIXEL(255, 0, 0), 1);
			}
		}

		found += qrean_detector_try_decode_qr(mono, candidates, num_candidates, on_found, NULL);
		found += qrean_detector_try_decode_mqr(mono, candidates, num_candidates, on_found, NULL);
		found += qrean_detector_try_decode_rmqr(mono, candidates, num_candidates, on_found, NULL);
		found += qrean_detector_try_decode_tqr(mono, candidates, num_candidates, on_found, NULL);

		found += qrean_detector_scan_barcodes(mono, on_found, NULL);
	}
	gettimeofday(&tv2, NULL);

	if (flag_bench) {
		fprintf(stderr, "%.1f ms\n", ((tv2.tv_sec - tv.tv_sec) * 1000.0 + (tv2.tv_usec - tv.tv_usec) / 1000.0) / n);
	}

	if (flag_debug && !found) {
		fprintf(stderr, "Not found\n");
	}
	if (debug_out) image_save_as_png(detected, debug_out);
}

void draw_pixel(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint8_t rgba[4])
{
	uint8_t r = rgba[0];
	uint8_t g = rgba[1];
	uint8_t b = rgba[2];

	image_draw_pixel(img, POINT(x, y), PIXEL(r, g, b));
}

void init_screen(pngle_t *pngle, uint32_t w, uint32_t h)
{
	W = w;
	H = h;
	img = new_image(W, H);
}

const char *progname;
int usage(FILE *out)
{
	fprintf(out, "Usage: %s [OPTION]... [FILE]\n", progname);
	fprintf(out, "Read QR/Barcode from PNG image file\n");
	fprintf(out, "\n");
	fprintf(out, "  General options:\n");
	fprintf(out, "    -h                Display this help\n");
	fprintf(out, "    -o FILENAME       Result text save as FILENAME\n");
	fprintf(out, "    -v                Verbose\n");
	fprintf(out, "\n");
	fprintf(out, "  Image processing options:\n");
	fprintf(out, "    -g GAMMA          Set gamma value (default: 1.8)\n");
	fprintf(out, "\n");
	fprintf(out, "  ECI options:\n");
	fprintf(out, "    -E CODE           Set initial ECI code\n");
	fprintf(out, "    -S                ShiftJIS mode (-E 20)\n");
	fprintf(out, "    -U                UTF-8 mode (-E 26)\n");
	fprintf(out, "\n");
	fprintf(out, "  Debug options:\n");
	fprintf(out, "    -D                Enable debug output\n");
	fprintf(out, "    -O FILENAME       Debug image save as FILENAME\n");

	return 1;
}

int main(int argc, char *argv[])
{
	progname = argv[0];
	extern int optind;
	extern char *optarg;

	char buf[1024];
	size_t remain = 0;
	int len;
	int ch;

	while ((ch = getopt(argc, argv, "ho:g:DO:vBUSE:")) != -1) {
		switch (ch) {
		case 'h':
			return usage(stdout);

		case 'o':
			out = fopen(optarg, "w+t");
			if (out == NULL) {
				fprintf(stderr, "fopen failed. (%s: %s)", optarg, strerror(errno));
				return 1;
			}
			break;

		case 'g':
			gamma_value = atof(optarg);
			break;

		case 'D':
			flag_debug = 1;
			qrean_on_debug_vprintf(vfprintf, stderr);
			break;

		case 'O':
			debug_out = fopen(optarg, "w+b");
			if (debug_out == NULL) {
				fprintf(stderr, "fopen failed. (%s: %s)", optarg, strerror(errno));
				return 1;
			}
			break;

		case 'B':
			flag_bench = 1;
			break;

		case 'v':
			flag_verbose = 1;
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

	FILE *in = stdin;

	if (argc == 0) {
		if (isatty(fileno(in))) {
			return usage(stderr);
		}
	} else {
		in = fopen(argv[0], "rb");
		if (in == NULL) {
			fprintf(stderr, "fopen failed. (%s: %s)", optarg, strerror(errno));
			return 1;
		}
	}

	if (debug_out == NULL && flag_debug && !isatty(fileno(stdout))) {
		debug_out = stdout;
	}
	if (out == NULL) out = flag_debug ? stderr : stdout;

	pngle_t *pngle = pngle_new();

	pngle_set_init_callback(pngle, init_screen);
	pngle_set_draw_callback(pngle, draw_pixel);
	pngle_set_done_callback(pngle, done);

	while (!feof(in)) {
		if (remain >= sizeof(buf)) {
			return -1;
		}

		len = fread(buf + remain, 1, sizeof(buf) - remain, in);
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
