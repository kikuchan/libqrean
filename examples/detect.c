#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// https://github.com/kikuchan/pngle
#include "pngle.h"

#include "hexdump.h"
#include "qrmatrix.h"
#include "qrpayload.h"
#include "runlength.h"

#include "image.h"

image_t *img;

uint32_t W = 0;
uint32_t H = 0;

typedef struct {
	int_fast32_t cx;
	int_fast32_t cy;
	uint_fast8_t module_size;
} candidate_t;
candidate_t candidates[10];
int num_candidates = 0;

void tryDecode(image_t *img, candidate_t *a, candidate_t *b, candidate_t *c) {
	// ABu + ACv
	qrmatrix_t *qr = new_qrmatrix();

	for (int version = 1; version <= 40; version++) {
		qrmatrix_set_version(qr, version);
		int size = qr->symbol_size;

		double alpha = 3.0;
		double beta = 7;
		double gamma = 0.5;

		for (double u = 0; u < size; u += 1) {
			for (double v = 0; v < size; v += 1) {
				int x = a->cx + (b->cx - a->cx) * (u - alpha) / (double)(size - beta) + (c->cx - a->cx) * (v - alpha) / (double)(size - beta) + gamma;
				int y = a->cy + (b->cy - a->cy) * (u - alpha) / (double)(size - beta) + (c->cy - a->cy) * (v - alpha) / (double)(size - beta) + gamma;

				uint32_t pix = image_read_pixel(img, x, y);
				qrmatrix_write_pixel(qr, u, v, pix == 0 ? 1 : 0);
			}
		}

		if (qrmatrix_set_format_info(qr, qrmatrix_read_format_info(qr))) {
			char buf[1024];

			qrformat_t f = qrmatrix_read_format_info(qr);
			fprintf(stderr, "formatinfo; %d %d\n", f.mask, f.level);

			if (qrmatrix_fix_errors(qr) >= 0) {
				qrpayload_t *payload = new_qrpayload(qr->version, qr->level);
				qrmatrix_read_payload(qr, payload);

				size_t l = qrmatrix_read_string(qr, buf, sizeof(buf));
				fprintf(stderr, "RECV: (%ld) '%s'\n", l, buf);
				qrmatrix_dump(qr);
			}
		}
	}
}

void done(pngle_t *pngle) {
	for (int y = 0; y < H; y++) {
		runlength_t rl = create_runlength();

		for (int x = 0; x < W; x++) {
			uint32_t v = image_read_pixel(img, x, y);
			if (v != 0 && v != MAKE_RGB(255, 255, 255)) continue;
			if (!runlength_push_value(&rl, v)) continue;

			if (!v || !runlength_match_ratio(&rl, 6, 1, 1, 3, 1, 1, 0)) continue;

			// assert almost [n, n, 3n, n, n, 1]
			int len = runlength_sum(&rl, 1, 6);

			int cx = x - 1 - (len / 2);
			int modsize = len / 7;
			int cy = y;


			runlength_t rlvu = create_runlength();
			runlength_t rlvd = create_runlength();
			int found_u = 0, found_d = 0;

			for (int yy = 0; yy < modsize * 12 / 2; yy++) {
				if (!found_u && runlength_push_value(&rlvu, image_read_pixel(img, cx, cy - yy))) {
					if (runlength_match_ratio(&rlvu, 4, 3, 2, 2, 0)) {
						found_u = runlength_get_count(&rl, 1);
					}
				}
				if (!found_d && runlength_push_value(&rlvd, image_read_pixel(img, cx, cy + yy))) {
					if (runlength_match_ratio(&rlvd, 4, 3, 2, 2, 0)) {
						found_d = runlength_get_count(&rl, 1);
					}
				}
			}
			if (found_u && found_d) {
				// FOUND

				// XXX: alter source image to avoid detecting multiple times at the almost the same position of the marker.
				image_extent_t r = image_paint(img, cx, cy, MAKE_RGB(0, 255, 0));
				image_extent_dump(&r);

				cx = (r.left + r.right) / 2;
				cy = (r.top + r.bottom) / 2;

				candidate_t cand = { cx, cy, modsize };
				image_write_filled_ellipse(img, cx, cy, 2, 2, MAKE_RGB(255, 0, 0));
				candidates[num_candidates++] = cand;
			}
		}
	}

	for (int i = 0; i < num_candidates; i++) {
		for (int j = i + 1; j < num_candidates; j++) {
			for (int k = j + 1; k < num_candidates; k++) {
				int idx[] = { i, j, k };

				for (int l = 0; l < 3; l++) {
					image_t *clone = image_clone(img);
					tryDecode(
						img,
						&candidates[idx[(l + 0) % 3]],
						&candidates[idx[(l + 1) % 3]],
						&candidates[idx[(l + 2) % 3]]
					);
					image_free(clone);
				}
			}
		}
	}
}

double gamma_value = 1.8;

void draw_pixel(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint8_t rgba[4]) {
	uint8_t b = rgba[2];
	uint8_t g = rgba[1];
	uint8_t r = rgba[0];

	// make it monochrome
	uint8_t v = pow((0.299 * r + 0.587 * g + 0.114 * b) / 255.0, 1 / gamma_value) * 255.0 < 127 ? 0 : 255;

	image_write_pixel(img, x, y, MAKE_RGB(v, v, v));
}

void init_screen(pngle_t *pngle, uint32_t w, uint32_t h) {
	W = w;
	H = h;
	img = new_image(W, H);
}

// % convert -auto-threshold otsu input.png - | ./decode3

int main(int argc, char *argv[]) {
	char buf[1024];
	size_t remain = 0;
	int len;

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
