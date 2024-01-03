#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// https://github.com/kikuchan/pngle
#include "pngle.h"

#include "qrean.h"
#include "qrpayload.h"
#include "qrspec.h"
#include "runlength.h"
#include "detector.h"
#include "debug.h"

#include "image.h"

image_t *img;

// #define DEBUG_DETECT

#ifdef DEBUG_DETECT
image_t *detected;
#endif

uint32_t W = 0;
uint32_t H = 0;

#ifdef DEBUG_DETECT
bit_t write_image_pixel(qrean_t *qrean, bitpos_t x, bitpos_t y, bitpos_t pos, bit_t value, void *opaque) {
	qrean_detector_perspective_t *warp = (qrean_detector_perspective_t *)opaque;
	image_draw_pixel(detected, image_point_transform(POINT(x, y), warp->h), value ? PIXEL(255, 0, 0) : PIXEL(0, 255, 0));
	//image_draw_filled_ellipse(detected, image_point_transform(POINT(x, y), warp->h), 1, 1, value ? PIXEL(255, 0, 0) : PIXEL(0, 255, 0));
	return 1;
}
#endif


static void on_found(qrean_detector_perspective_t *warp, void *opaque) {
	char buffer[1024];

#ifdef DEBUG_DETECT
	if (QREAN_IS_TYPE_QR(warp->qrean)) {
		image_point_t points[] = {
			image_point_transform(POINT(0, 0), warp->h),
			image_point_transform(POINT(warp->qrean->canvas.symbol_width - 1, 0), warp->h),
			image_point_transform(POINT(warp->qrean->canvas.symbol_width - 1, warp->qrean->canvas.symbol_height - 1), warp->h),
			image_point_transform(POINT(0, warp->qrean->canvas.symbol_height - 1), warp->h),
		};
		image_draw_polygon(detected, 4, points, PIXEL(0, 255, 0), 1);

		qrean_on_write_pixel(warp->qrean, write_image_pixel, warp);
		qrpayload_t payload = create_qrpayload(warp->qrean->qr.version, warp->qrean->qr.level);
		qrean_read_qr_payload(warp->qrean, &payload);
		qrean_write_frame(warp->qrean);
		qrean_write_qr_payload(warp->qrean, &payload);
	}
#endif

	qrean_read_string(warp->qrean, buffer, sizeof(buffer));
#ifdef DEBUG_DETECT
	fprintf(stderr, "%s\n", buffer);
#else
	printf("%s\n", buffer);
#endif
}

void done(pngle_t *pngle) {
	image_t *mono = image_clone(img);

	image_digitize(mono, img, 1.8);
	//image_morphology_open(mono);
	//image_morphology_close(mono);

	int num_candidates;
	qrean_detector_qr_finder_candidate_t *candidates = qrean_detector_scan_qr_finder_pattern(mono, &num_candidates);
	int found = 0;

#ifdef DEBUG_DETECT
	detected = image_clone(mono);
	for (int i = 0; i < num_candidates; i++) {
		image_draw_filled_ellipse(detected, candidates[i].center, 5, 5, PIXEL(255, 0, 0));
		image_draw_extent(detected, candidates[i].extent, PIXEL(255, 0, 0), 1);
	}
#endif

	found += qrean_detector_try_decode_qr(mono, candidates, num_candidates, on_found, NULL);
	found += qrean_detector_try_decode_mqr(mono, candidates, num_candidates, on_found, NULL);
	found += qrean_detector_try_decode_rmqr(mono, candidates, num_candidates, on_found, NULL);
	found += qrean_detector_try_decode_tqr(mono, candidates, num_candidates, on_found, NULL);

	if (!found) {
#ifndef DEBUG_DETECT
		fprintf(stderr, "Not found\n");
#endif
	}


	qrean_detector_scan_barcodes(mono, on_found, NULL);
#ifdef DEBUG_DETECT
	image_dump(detected, stdout);
#endif
}

double gamma_value = 1.8;

void draw_pixel(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint8_t rgba[4]) {
	uint8_t r = rgba[0];
	uint8_t g = rgba[1];
	uint8_t b = rgba[2];

	image_draw_pixel(img, POINT(x, y), PIXEL(r, g, b));
}

void init_screen(pngle_t *pngle, uint32_t w, uint32_t h) {
	W = w;
	H = h;
	img = new_image(W, H);
}

int main(int argc, char *argv[]) {
	char buf[1024];
	size_t remain = 0;
	int len;

#ifdef DEBUG_DETECT
	qrean_on_debug_vprintf(vfprintf, stderr);
#endif

	FILE *fp = stdin;

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
