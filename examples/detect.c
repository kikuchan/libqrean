#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// https://github.com/kikuchan/pngle
#include "pngle.h"

#include "qrmatrix.h"
#include "qrpayload.h"
#include "qrspec.h"
#include "runlength.h"
#include "qrdetector.h"
#include "debug.h"

#include "image.h"

image_t *img;

//#define DEBUG_DETECT

#ifdef DEBUG_DETECT
image_t *detected;
#endif

uint32_t W = 0;
uint32_t H = 0;

#ifdef DEBUG_DETECT
bit_t write_image_pixel(qrmatrix_t *qr, bitpos_t x, bitpos_t y, bitpos_t pos, bit_t value, void *opaque) {
	qrdetector_perspective_t *warp = (qrdetector_perspective_t *)opaque;
	image_draw_pixel(detected, image_point_transform(POINT(x, y), warp->h), value ? PIXEL(255, 0, 0) : PIXEL(0, 255, 0));
	//image_draw_filled_ellipse(detected, image_point_transform(POINT(x, y), warp->h), 1, 1, value ? PIXEL(255, 0, 0) : PIXEL(0, 255, 0));
	return 1;
}
#endif


int tryDecode(image_t *imgsrc, image_point_t src[3]) {
	image_t *img = image_clone(imgsrc);
	qrmatrix_t *qr = new_qrmatrix();
	int found = 0;

	for (int version = 1; version <= 40 && !found; version++) {
		qrmatrix_set_version(qr, version);

		qrdetector_perspective_t warp = create_qrdetector_perspective(qr, img);
		qrdetector_perspective_setup_by_finder_pattern_qr(&warp, src);

#ifdef DEBUG_DETECT
		qrmatrix_on_write_pixel(qr, write_image_pixel, &warp);
#endif

		if (qrmatrix_read_finder_pattern(qr, 0) > 10) continue;

		if (qrdetector_perspective_fit_by_alignment_pattern(&warp)) {
			// fprintf(stderr, "   ** Ajusted\n");
		}

		if (qrmatrix_set_format_info(qr, qrmatrix_read_format_info(qr))) {
			char buf[1024];

			if (qrmatrix_fix_errors(qr) >= 0) {
				size_t l = qrmatrix_read_string(qr, buf, sizeof(buf));
#ifndef DEBUG_DETECT
				qrmatrix_dump(qr, stderr);
				printf("Found: RECV(%zu): '%s'\n", l, buf);
#else
				safe_fprintf(stderr, "RECV(%zu): '%s'\n", l, buf);

				// draw read points on `detected` image for debug
				qrmatrix_write_finder_pattern(qr);
				qrmatrix_write_alignment_pattern(qr);
				qrmatrix_write_timing_pattern(qr);
				qrmatrix_write_format_info(qr);

				qrpayload_t *payload = new_qrpayload(qr->version, qr->level);
				qrmatrix_read_payload(qr, payload);
				qrmatrix_write_payload(qr, payload);
				qrpayload_free(payload);
#endif

				found = 1;
			}
		}
	}

	qrmatrix_free(qr);

	return found;
}


void done(pngle_t *pngle) {
	image_t *mono = image_clone(img);

	image_digitize(mono, img, 1.8);

	int num_candidates;
	qrdetector_finder_candidate_t *candidates = qrdetector_scan_finder_pattern(mono, &num_candidates);
	int found = 0;

#ifdef DEBUG_DETECT
	detected = image_clone(mono);
	for (int i = 0; i < num_candidates; i++) {
		image_draw_filled_ellipse(detected, candidates[i].center, 5, 5, PIXEL(255, 0, 0));
		image_draw_extent(detected, candidates[i].extent, PIXEL(255, 0, 0), 1);
	}
#endif

	// try all possible pairs!
	for (int i = 0; i < num_candidates; i++) {
		for (int j = i + 1; j < num_candidates; j++) {
			for (int k = j + 1; k < num_candidates; k++) {
				int idx[] = { i, j, k, i, k, j, j, i, k, j, k, i, k, i, j, k, j, i };
				for (int l = 0; l < 6; l++) {
					image_point_t points[] = {
						candidates[idx[l * 3 + 0]].center,
						candidates[idx[l * 3 + 1]].center,
						candidates[idx[l * 3 + 2]].center,
					};

					found += tryDecode(mono, points);
				}
			}
		}
	}

	if (!found) {
#ifndef DEBUG_DETECT
		printf("Not found\n");
#endif
	}
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
