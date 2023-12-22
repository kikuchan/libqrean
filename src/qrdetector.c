#include "qrdetector.h"
#include "image.h"
#include "qrspec.h"
#include "runlength.h"
#include "utils.h"

qrdetector_candidate_t *qrdetector_scan_finder_pattern(image_t *src, int *found) {
	static qrdetector_candidate_t candidates[MAX_CANDIDATES];
	int candidx = 0;

	image_t *img = image_clone(src);

	for (int y = 0; y < (int)img->height; y++) {
		runlength_t rl = create_runlength();

		for (int x = 0; x < (int)img->width; x++) {
			uint32_t v = image_read_pixel(img, POINT(x, y));
			if (v != 0 && v != PIXEL(255, 255, 255)) continue;
			if (!runlength_push_value(&rl, v)) continue;

			if (!v || !runlength_match_ratio(&rl, 6, 1, 1, 3, 1, 1, 0)) continue;

			// assert almost [n, n, 3n, n, n, 1]

			int cx = x - runlength_sum(&rl, 0, 4) + runlength_get_count(&rl, 3) / 2; // dark module on the center
			int lx = x - runlength_sum(&rl, 0, 6) + runlength_get_count(&rl, 5) / 2; // dark module on the left ring
			int rx = x - runlength_sum(&rl, 0, 2) + runlength_get_count(&rl, 1) / 2; // dark module on the right ring
			int len = runlength_sum(&rl, 1, 5);
			float modsize_x = len / 7.0;
			int cy = y;

			// not a dark module
			if (image_read_pixel(img, POINT(cx, cy)) != 0) continue;
			if (image_read_pixel(img, POINT(lx, cy)) != 0) continue;
			if (image_read_pixel(img, POINT(rx, cy)) != 0) continue;

			// check vertical runlength
			runlength_t rlvu = create_runlength();
			runlength_t rlvd = create_runlength();
			int found_u = 0, found_d = 0;
			for (int yy = 0; yy < len; yy++) {
				if (!found_u && runlength_push_value(&rlvu, image_read_pixel(img, POINT(cx, cy - yy)))) {
					if (runlength_match_ratio(&rlvu, 4, 3, 2, 2, 0)) {
						found_u = runlength_sum(&rlvu, 1, 3);
					}
				}
				if (!found_d && runlength_push_value(&rlvd, image_read_pixel(img, POINT(cx, cy + yy)))) {
					if (runlength_match_ratio(&rlvd, 4, 3, 2, 2, 0)) {
						found_d = runlength_sum(&rlvd, 1, 3);
					}
				}
			}

			if (found_u && found_d) {
				image_pixel_t inner_block_color = PIXEL(0, 255, 0);
				image_pixel_t ring_color = PIXEL(255, 0, 0);

				// mark visit flag by painting inner most block
				image_paint_result_t r = image_paint(img, POINT(cx, cy), inner_block_color);
				float modsize_y = (found_u + found_d - 1) / 7.0;

				// let's make sure they are disconnected from the inner most block
				if (image_read_pixel(img, POINT(lx, cy)) != PIXEL(0, 0, 0)) continue;
				if (image_read_pixel(img, POINT(rx, cy)) != PIXEL(0, 0, 0)) continue;

				// let's make sure they are connected
				image_paint(img, POINT(lx, cy), ring_color);
				if (image_read_pixel(img, POINT(lx, cy)) != image_read_pixel(img, POINT(rx, cy))) continue;

				float real_cx = (r.extent.left + r.extent.right) / 2.0f;
				float real_cy = (r.extent.top + r.extent.bottom) / 2.0f;

				if (candidx < MAX_CANDIDATES) {
					candidates[candidx].center = POINT(real_cx, real_cy);
					candidates[candidx].extent = r.extent;
					candidates[candidx].area = r.area;
					candidates[candidx].center_x = real_cx;
					candidates[candidx].center_y = real_cy;
					candidates[candidx].modsize_x = modsize_x;
					candidates[candidx].modsize_y = modsize_y;
					candidx++;
				}
			}
		}
	}

	image_free(img);

	candidates[candidx].center = POINT_INVALID;
	candidates[candidx].modsize_x = 0;
	candidates[candidx].modsize_y = 0;

	if (found) *found = candidx;

	return candidates;
}

qrdetector_perspective_t create_qrdetector_perspective(qrmatrix_t *qr, image_t *img) {
	qrdetector_perspective_t warp = {
		.qr = qr,
		.img = img,
	};
	return warp;
}

static bit_t qrdetector_perspective_read_image_pixel(qrmatrix_t *qr, bitpos_t x, bitpos_t y, bitpos_t pos, void *opaque) {
	qrdetector_perspective_t *warp = (qrdetector_perspective_t *)opaque;

	uint32_t pix = image_read_pixel(warp->img, image_point_transform(POINT(x, y), warp->h));

	return pix == 0 ? 1 : 0;
}

void qrdetector_perspective_setup_by_finder_pattern(qrdetector_perspective_t *warp, image_point_t src[3]) {
	warp->src[0] = POINT(3, 3),                                                     // 1st keystone
		warp->src[1] = POINT(warp->qr->symbol_size - 4, 3),                         // 2nd keystone
		warp->src[2] = POINT(3, warp->qr->symbol_size - 4),                         // 3rd keystone
		warp->src[3] = POINT(warp->qr->symbol_size - 4, warp->qr->symbol_size - 4), // 4th estimated pseudo keystone

		warp->dst[0] = src[0];
	warp->dst[1] = src[1];
	warp->dst[2] = src[2];
	warp->dst[3] = POINT(POINT_X(src[0]) + (POINT_X(src[1]) - POINT_X(src[0])) + (POINT_X(src[2]) - POINT_X(src[0])),
	                     POINT_Y(src[0]) + (POINT_Y(src[1]) - POINT_Y(src[0])) + (POINT_Y(src[2]) - POINT_Y(src[0])));

	warp->h = create_image_transform_matrix(warp->src, warp->dst);

	qrmatrix_on_read_pixel(warp->qr, qrdetector_perspective_read_image_pixel, warp);
}

int qrdetector_perspective_fit_by_alignment_pattern(qrdetector_perspective_t *d) {
	image_t *img = image_clone(d->img);
	// image_t *img = d->img;
	int adjusted = 0;

	int N = qrspec_get_alignment_num(d->qr->version);
	int i = N - 1;
	{
		int cx = qrspec_get_alignment_position_x(d->qr->version, i);
		int cy = qrspec_get_alignment_position_y(d->qr->version, i);

		image_point_t c = image_point_transform(POINT(cx, cy), d->h);
		image_point_t next = image_point_transform(POINT(cx + 1, cy), d->h);
		float module_size = image_point_norm(next - c);

		int dist = 5;
		for (float y = cy - dist; y < cy + dist; y += 0.2) {
			for (float x = cx - dist; x < cx + dist; x += 0.2) {
				image_point_t p = image_point_transform(POINT(x, y), d->h);
				image_point_t ring = image_point_transform(POINT(x + 1, y), d->h);
				if (image_read_pixel(img, p) == 0 && image_read_pixel(img, ring) == PIXEL(255, 255, 255)) {
					image_paint_result_t result = image_paint(img, ring, PIXEL(255, 0, 0));
					if (module_size * module_size < result.area && result.area < module_size * module_size * 16) {
						image_point_t new_center = image_extent_center(&result.extent);

						qrdetector_perspective_t backup = *d;

						d->src[3] = POINT(cx, cy);
						d->dst[3] = new_center;
						d->h = create_image_transform_matrix(d->src, d->dst);

						if (qrmatrix_read_alignment_pattern(d->qr, i) < 10) {
							image_paint(img, ring, PIXEL(0, 255, 0));
							adjusted = 1;

							goto next;
						}

						// restore if it doesn't fit
						*d = backup;
					}
				}
			}
		}
	next:;
	}

	//	image_dump(img, stdout);
	//	qrmatrix_dump(qr);

	return adjusted;
}
