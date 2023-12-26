#include "qrdetector.h"
#include "image.h"
#include "qrean.h"
#include "qrspec.h"
#include "runlength.h"
#include "utils.h"
#include <math.h>

qrdetector_finder_candidate_t *qrdetector_scan_finder_pattern(image_t *src, int *found)
{
	static qrdetector_finder_candidate_t candidates[MAX_CANDIDATES];
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

			int cx = x - runlength_sum(&rl, 1, 3) + runlength_get_count(&rl, 3) / 2; // dark module on the center
			int lx = x - runlength_sum(&rl, 1, 5) + runlength_get_count(&rl, 5) / 2; // dark module on the left ring
			int rx = x - runlength_sum(&rl, 1, 1) + runlength_get_count(&rl, 1) / 2; // dark module on the right ring

			int len = runlength_sum(&rl, 1, 5);
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
			if (!found_u && runlength_match_ratio(&rlvu, 4, 3, 2, 2, 0)) {
				found_u = runlength_sum(&rlvu, 1, 3);
			}
			if (!found_d && runlength_match_ratio(&rlvd, 4, 3, 2, 2, 0)) {
				found_d = runlength_sum(&rlvd, 1, 3);
			}

			if (candidx < MAX_CANDIDATES && found_u && found_d) {
				image_pixel_t inner_block_color = PIXEL(0, 255, 0);
				image_pixel_t ring_color = PIXEL(255, 0, 0);

				// mark visit flag by painting inner most block
				image_paint_result_t inner_block = image_paint(img, POINT(cx, cy), inner_block_color);

				// let's make sure they are disconnected from the inner most block
				if (image_read_pixel(img, POINT(lx, cy)) != PIXEL(0, 0, 0)) {
					continue;
				}
				if (image_read_pixel(img, POINT(rx, cy)) != PIXEL(0, 0, 0)) {
					continue;
				}

				// let's make sure they are connected
				image_paint_result_t ring = image_paint(img, POINT(lx, cy), ring_color);
				if (image_read_pixel(img, POINT(lx, cy)) != image_read_pixel(img, POINT(rx, cy))) {
					// paint it back ;)
					image_paint(img, POINT(lx, cy), PIXEL(0, 0, 0));
					continue;
				}

				float real_cx = (ring.extent.left + ring.extent.right) / 2.0f;
				float real_cy = (ring.extent.top + ring.extent.bottom) / 2.0f;

				// find ring corners
				int w = ring.extent.right - ring.extent.left;
				int h = ring.extent.bottom - ring.extent.top;
				int cidx = 0;
				float corners[4] = { NAN, NAN, NAN, NAN };

				for (float r = sqrt(w * w + h * h) * 1.1; r > 0 && cidx < 4; r -= 0.2) {
					for (float theta = 0; theta < 2 * M_PI && cidx < 4; theta += 1.0 / r) {
						float x = floor(cx + r * sin(theta));
						float y = floor(cy - r * cos(theta));

						if (image_read_pixel(img, POINT(x, y)) == ring_color) {
							int slot = !cidx ? 0 : floor(fmod(theta + 2 * M_PI + M_PI_4 - corners[0], 2 * M_PI) / M_PI_2);
							assert(0 <= slot && slot < 4);

							if (isnan(corners[slot])) {
								candidates[candidx].corners[slot] = POINT(x, y);
								corners[slot] = theta;
								cidx++;
							}
						}
					}
				}

				if (cidx < 4) {
					// No corners... paint it back
					image_paint(img, POINT(lx, cy), PIXEL(0, 0, 0));
					continue;
				}

				candidates[candidx].center = POINT(real_cx, real_cy);
				candidates[candidx].extent = ring.extent;
				candidates[candidx].area = inner_block.area;
				candidx++;
			}
		}
	}

	image_free(img);

	// guardian
	candidates[candidx].center = POINT(NAN, NAN);

	if (found) *found = candidx;

	return candidates;
}

qrdetector_perspective_t create_qrdetector_perspective(qrean_t *qrean, image_t *img)
{
	qrdetector_perspective_t warp = {
		.qrean = qrean,
		.img = img,
	};
	return warp;
}

static bit_t qrdetector_perspective_read_image_pixel(qrean_t *qrean, bitpos_t x, bitpos_t y, bitpos_t pos, void *opaque)
{
	qrdetector_perspective_t *warp = (qrdetector_perspective_t *)opaque;

	uint32_t pix = image_read_pixel(warp->img, image_point_transform(POINT(x, y), warp->h));

	return pix == 0 ? 1 : 0;
}

void qrdetector_perspective_setup_by_finder_pattern_qr(qrdetector_perspective_t *warp, image_point_t src[3])
{
	warp->src[0] = POINT(3, 3); // center of the 1st keystone
	warp->src[1] = POINT(warp->qrean->canvas.symbol_width - 4, 3); // center of the 2nd keystone
	warp->src[2] = POINT(3, warp->qrean->canvas.symbol_height - 4); // center of the 3rd keystone
	warp->src[3]
		= POINT(warp->qrean->canvas.symbol_width - 4, warp->qrean->canvas.symbol_height - 4); // center of the 4th estimated pseudo keystone

	warp->dst[0] = src[0];
	warp->dst[1] = src[1];
	warp->dst[2] = src[2];
	warp->dst[3] = POINT(POINT_X(src[0]) + (POINT_X(src[1]) - POINT_X(src[0])) + (POINT_X(src[2]) - POINT_X(src[0])),
		POINT_Y(src[0]) + (POINT_Y(src[1]) - POINT_Y(src[0])) + (POINT_Y(src[2]) - POINT_Y(src[0])));

	warp->h = create_image_transform_matrix(warp->src, warp->dst);

	qrean_on_read_pixel(warp->qrean, qrdetector_perspective_read_image_pixel, warp);
}

void qrdetector_perspective_setup_by_finder_pattern_ring_corners(qrdetector_perspective_t *warp, image_point_t ring[4], int offset)
{
	float modsize = image_point_distance(ring[(0 + offset) % 4], ring[(1 + offset) % 4]) / 7.0;
	float d = 0.5 / modsize;

	warp->src[0] = POINT(-0.5 + d, -0.5 + d); // left--top of the ring
	warp->src[1] = POINT(6.5 - d, -0.5 + d); // right-top of the ring
	warp->src[2] = POINT(6.5 - d, 6.5 - d); // right-bottom of the ring
	warp->src[3] = POINT(-0.5 + d, 6.5 - d); // left--bottom of the ring

	warp->dst[0] = ring[(0 + offset) % 4];
	warp->dst[1] = ring[(1 + offset) % 4];
	warp->dst[2] = ring[(2 + offset) % 4];
	warp->dst[3] = ring[(3 + offset) % 4];

	warp->h = create_image_transform_matrix(warp->src, warp->dst);

	qrean_on_read_pixel(warp->qrean, qrdetector_perspective_read_image_pixel, warp);
}

int qrdetector_perspective_fit_by_alignment_pattern(qrdetector_perspective_t *d)
{
	image_t *img = image_clone(d->img);
	// image_t *img = d->img;
	int adjusted = 0;

	int N = qrspec_get_alignment_num(d->qrean->qr.version);
	int i = N - 1;
	if (i >= 0) {
		int cx = qrspec_get_alignment_position_x(d->qrean->qr.version, i);
		int cy = qrspec_get_alignment_position_y(d->qrean->qr.version, i);

		image_point_t c = image_point_transform(POINT(cx, cy), d->h);
		image_point_t next = image_point_transform(POINT(cx + 1, cy), d->h);
		float module_size = image_point_distance(next, c);

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

						if (qrean_read_qr_alignment_pattern(d->qrean, i) < 10) {
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

	return adjusted;
}
