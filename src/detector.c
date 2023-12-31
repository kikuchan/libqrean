#include <math.h>

#include "debug.h"
#include "detector.h"
#include "image.h"
#include "qrean.h"
#include "qrspec.h"
#include "qrtypes.h"
#include "runlength.h"
#include "utils.h"

int qrean_detector_scan_barcodes(image_t *src, void (*on_found)(qrean_detector_perspective_t *warp, void *opaque), void *opaque)
{
	int found = 0;
	image_t *img = image_clone(src);

	for (int y = 0; y < (int)img->height; y++) {
		runlength_t rl = create_runlength();
		for (int x = 0; x < (int)img->width; x++) {
			uint32_t v = image_read_pixel(img, POINT(x, y)) & 0xFFFFFF; // drop alpha channel
			if (v != 0 && v != PIXEL(255, 255, 255)) {
				runlength_init(&rl);
				continue;
			}
			if (!runlength_push_value(&rl, v)) continue;
			if (!v) continue;

			// EAN: 101                     1, 1, 1
			// CODE39: 100010111011101      1, 3, 1, 1, 3, 1, 3, 1, 1
			// CODE93: 10101111             1, 1, 1, 1, 4
			// ITF: 101                     1, 1, 1
			// NW7-A: 1011100010001         1, 1, 3, 3, 1, 3, 1
			// NW7-B: 1000100010111         1, 3, 1, 3, 1, 1, 3
			// NW7-C: 1010001000111         1, 1, 1, 3, 1, 3, 3
			// NW7-D: 1010001110001         1, 1, 1, 3, 3, 3, 1

			int n = 0;
			float barsize = 0;

			if (runlength_match_ratio(&rl, 10, 1, 3, 1, 1, 3, 1, 3, 1, 1, 0)) {
				// CODE39
				n = runlength_sum(&rl, 1, 9);
				barsize = n / (6.0 + 3 * 3);
				if (runlength_get_count(&rl, 10) < 2 * barsize) continue; // quiet zone required
			} else if (runlength_match_ratio(&rl, 8, 1, 1, 3, 3, 1, 3, 1, 0) || runlength_match_ratio(&rl, 8, 1, 3, 1, 3, 1, 1, 3, 0)
				|| runlength_match_ratio(&rl, 8, 1, 1, 1, 3, 1, 3, 3, 0) || runlength_match_ratio(&rl, 8, 1, 1, 1, 3, 3, 3, 1, 0)) {
				// NW7
				n = runlength_sum(&rl, 1, 7);
				barsize = n / (4.0 + 3 * 3);
				if (runlength_get_count(&rl, 8) < 2 * barsize) continue; // quiet zone required
			} else if (runlength_match_ratio(&rl, 6, 1, 1, 1, 1, 4, 0)) {
				// CODE93
				n = runlength_sum(&rl, 1, 5);
				barsize = n / 8.0;
				if (runlength_get_count(&rl, 6) < 2 * barsize) continue; // quiet zone required
			} else if (runlength_match_ratio(&rl, 4, 1, 1, 1, 0)) {
				// EAN or ITF
				n = runlength_sum(&rl, 1, 3);
				barsize = n / 3.0;
				if (runlength_get_count(&rl, 4) < 2 * barsize) continue; // quiet zone required
			} else {
				continue;
			}

			qrean_code_type_t codes[] = {
				QREAN_CODE_TYPE_EAN13,
				QREAN_CODE_TYPE_EAN8,
				QREAN_CODE_TYPE_UPCA,
				QREAN_CODE_TYPE_CODE39,
				QREAN_CODE_TYPE_CODE93,
				QREAN_CODE_TYPE_ITF,
				QREAN_CODE_TYPE_NW7,
			};

			image_transform_matrix_t mat = {
				{barsize, 0, x - n - 1 + barsize / 2, 0, 0, (float)y, 0, 0}
			};

			for (size_t i = 0; i < sizeof(codes) / sizeof(codes[0]); i++) {
				qrean_t *qrean = new_qrean(codes[i]);
				qrean_detector_perspective_t warp = create_qrean_detector_perspective(qrean, src);
				warp.h = mat;
				qrean_on_read_pixel(warp.qrean, qrean_detector_perspective_read_image_pixel, &warp);

				char buf[256];
				if (qrean_read_string(qrean, buf, sizeof(buf))) {
					on_found(&warp, opaque);
					found++;

					// paint dark bar to prevent the bar to be detected twice
					for (int xx = x - n - 1; xx < x; xx++) {
						uint32_t v = image_read_pixel(img, POINT(xx, y)) & 0xFFFFFF; // drop alpha channel
						if (!v) image_paint(img, POINT(xx, y), PIXEL(255, 0, 0));
					}
				}
				qrean_free(qrean);
			}
		}
	}

	image_free(img);

	return found;
}

qrean_detector_qr_finder_candidate_t *qrean_detector_scan_qr_finder_pattern(image_t *src, int *found)
{
	static qrean_detector_qr_finder_candidate_t candidates[MAX_CANDIDATES];
	int candidx = 0;

	image_t *img = image_clone(src);

	for (int y = 0; y < (int)img->height; y++) {
		runlength_t rl = create_runlength();

		for (int x = 0; x < (int)img->width; x++) {
			uint32_t v = image_read_pixel(img, POINT(x, y));
			if (v != 0 && v != PIXEL(255, 255, 255)) {
				runlength_init(&rl);
				continue;
			}
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
				if (image_read_pixel(img, POINT(real_cx, real_cy)) != inner_block_color) continue;

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

qrean_detector_perspective_t create_qrean_detector_perspective(qrean_t *qrean, image_t *img)
{
	qrean_detector_perspective_t warp = {
		.qrean = qrean,
		.img = img,
	};
	return warp;
}

bit_t qrean_detector_perspective_read_image_pixel(qrean_t *qrean, bitpos_t x, bitpos_t y, bitpos_t pos, void *opaque)
{
	qrean_detector_perspective_t *warp = (qrean_detector_perspective_t *)opaque;

	uint32_t pix = image_read_pixel(warp->img, image_point_transform(POINT(x, y), warp->h));

	return pix == 0 ? 1 : 0;
}

void qrean_detector_perspective_setup_by_qr_finder_pattern_centers(qrean_detector_perspective_t *warp, image_point_t src[3])
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

	qrean_on_read_pixel(warp->qrean, qrean_detector_perspective_read_image_pixel, warp);
}

void qrean_detector_perspective_setup_by_qr_finder_pattern_ring_corners(
	qrean_detector_perspective_t *warp, image_point_t ring[4], int offset)
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

	qrean_on_read_pixel(warp->qrean, qrean_detector_perspective_read_image_pixel, warp);
}

int qrean_detector_perspective_fit_for_qr(qrean_detector_perspective_t *warp)
{
	image_t *img = image_clone(warp->img);
	// image_t *img = warp->img;
	int adjusted = 0;

	int N = qrspec_get_alignment_num(warp->qrean->qr.version);
	int i = N - 1;
	if (i >= 0) {
		int cx = qrspec_get_alignment_position_x(warp->qrean->qr.version, i);
		int cy = qrspec_get_alignment_position_y(warp->qrean->qr.version, i);

		image_point_t c = image_point_transform(POINT(cx, cy), warp->h);
		image_point_t next = image_point_transform(POINT(cx + 1, cy), warp->h);
		float module_size = image_point_distance(next, c);

		int dist = 5;
		for (float y = cy - dist; y < cy + dist; y += 0.2) {
			for (float x = cx - dist; x < cx + dist; x += 0.2) {
				image_point_t p = image_point_transform(POINT(x, y), warp->h);
				image_point_t ring = image_point_transform(POINT(x + 1, y), warp->h);
				if (image_read_pixel(img, p) == 0 && image_read_pixel(img, ring) == PIXEL(255, 255, 255)) {
					image_paint_result_t result = image_paint(img, ring, PIXEL(255, 0, 0));
					if (module_size * module_size < result.area && result.area < module_size * module_size * 16) {
						image_point_t new_center = image_extent_center(&result.extent);

						qrean_detector_perspective_t backup = *warp;

						warp->src[3] = POINT(cx, cy);
						warp->dst[3] = new_center;
						warp->h = create_image_transform_matrix(warp->src, warp->dst);

						if (qrean_read_qr_alignment_pattern(warp->qrean, i) < 10) {
							image_paint(img, ring, PIXEL(0, 255, 0));
							adjusted = 1;

							goto next;
						}

						// restore if it doesn't fit
						*warp = backup;
					}
				}
			}
		}
	next:;
	}

	image_free(img);

	return adjusted;
}

int qrean_detector_try_decode_qr(image_t *src, qrean_detector_qr_finder_candidate_t *candidates, int num_candidates,
	void (*on_found)(qrean_detector_perspective_t *warp, void *opaque), void *opaque)
{
	int found = 0;
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

					qrean_t *qrean = new_qrean(QREAN_CODE_TYPE_QR);

					for (int version = QR_VERSION_1; version <= QR_VERSION_40; version++) {
						qrean_set_qr_version(qrean, (qr_version_t)version);

						qrean_detector_perspective_t warp = create_qrean_detector_perspective(qrean, src);
						qrean_detector_perspective_setup_by_qr_finder_pattern_centers(&warp, points);

						if (qrean_read_qr_finder_pattern(qrean, -1) > 10) continue;

						qrean_detector_perspective_fit_for_qr(&warp);

						if (qrean_set_qr_format_info(qrean, qrean_read_qr_format_info(qrean, -1))) {
							if (qrean_read_qr_version(qrean) != version) continue;

							if (qrean_fix_errors(qrean) >= 0) {
								on_found(&warp, opaque);
								found++;
								break;
							}
						}
					}

					qrean_free(qrean);
				}
			}
		}
	}

	return found;
}

static image_point_t find_corner(image_t *img, image_point_t c, image_pixel_t pix, float max_dist, float center_theta, float delta_theta)
{
	for (float r = max_dist; r > 0; r -= 0.2) {
		float theta;
		for (theta = center_theta - delta_theta; theta <= center_theta + delta_theta; theta += 1 / max_dist) {
			float x = c.x + r * cos(theta);
			float y = c.y + r * sin(theta);
			if (image_read_pixel(img, POINT(x, y)) == pix) {
				return POINT(x, y);
			}
		}
	}
	return c;
}

static image_point_t find_rmqr_corner_finder_pattern(
	qrean_detector_perspective_t *warp, image_point_t a, float angle, float dist, int corner_size)
{
	float modsize = image_point_distance(warp->dst[0], warp->dst[1]) / 7;

	image_point_t last_p = POINT_INVALID;
	int found = 0;

	for (float theta = angle - M_PI / 32; theta <= angle + M_PI / 32; theta += 1 / dist) {
		runlength_t rl = create_runlength();
		for (int i = 0; i < dist * 2; i += 1) {
			image_point_t p = POINT(a.x + i * cos(theta), a.y + i * sin(theta));
			image_pixel_t pix = image_read_pixel(warp->img, p) == 0 ? 1 : 0;
			if (runlength_push_value(&rl, pix)) {
				int ratio = round(runlength_get_count(&rl, 1) / modsize);
				if (ratio != 0 && ratio != 5 && ratio != 6 && ratio != 7 && ratio != 1 && ratio != 3) {
					break;
				}
			}

			if (runlength_match_ratio(&rl, 7, 1, 1, 1, 1, 1, corner_size, 2) && !pix) {
				float n = i - runlength_get_count(&rl, 0) - runlength_get_count(&rl, 0) / 2.0;
				image_point_t p = POINT(a.x + n * cos(theta), a.y + n * sin(theta));

				if (image_read_pixel(warp->img, p) != 0) continue;

				if (found < n) {
					image_t *work = image_clone(warp->img);
					image_paint_result_t painted = image_paint(work, p, PIXEL(255, 0, 0));
					image_point_t c = image_extent_center(&painted.extent);
					float dist = image_point_distance(c, p) * 1.2;
					p = find_corner(work, c, PIXEL(255, 0, 0), dist, image_point_angle(c, p), modsize * 1.5 / dist);
					image_free(work);

					found = n;
					last_p = p;
				}
			}
		}
	}

	return last_p;
}

int qrean_detector_perspective_fit_for_rmqr(qrean_detector_perspective_t *warp)
{
	image_point_t a = image_point_transform(POINT(0, 0), warp->h);
	image_point_t b = image_point_transform(POINT(warp->qrean->canvas.symbol_width - 1, 0), warp->h);
	image_point_t c = image_point_transform(POINT(warp->qrean->canvas.symbol_width - 1, warp->qrean->canvas.symbol_height - 1), warp->h);
	image_point_t d = image_point_transform(POINT(0, warp->qrean->canvas.symbol_height - 1), warp->h);
	float modsize = image_point_distance(warp->dst[0], warp->dst[1]) / 7;
	float delta = 0.5 / modsize;

	image_point_t rt = find_rmqr_corner_finder_pattern(warp, a, image_point_angle(a, b), image_point_distance(a, b), 3);
	image_point_t lb = find_rmqr_corner_finder_pattern(warp, a, image_point_angle(a, d), image_point_distance(a, d), 3);
	if (POINT_IS_INVALID(lb)) {
		image_point_t center = image_point_transform(POINT(3.5, 3.5), warp->h);
		float dist = image_point_distance(center, d) * 1.2;
		lb = find_corner(warp->img, center, PIXEL(0, 0, 0), dist, image_point_angle(center, d), modsize * 1.5 / dist);
	}
	image_point_t rb = find_rmqr_corner_finder_pattern(warp, lb, image_point_angle(d, c), image_point_distance(d, c), 5);
	image_point_t lt;
	{
		image_point_t center = image_point_transform(POINT(3.5, 3.5), warp->h);
		float dist = image_point_distance(center, a) * 1.2;
		lt = find_corner(warp->img, center, PIXEL(0, 0, 0), dist, image_point_angle(center, a), modsize * 1.5 / dist);
	}

	qrean_detector_perspective_t backup = *warp;

	if (!POINT_IS_INVALID(lt)) {
		warp->src[0] = POINT(-0.5 + delta, -0.5 + delta);
		warp->dst[0] = lt;
	}

	warp->src[1] = POINT(warp->qrean->canvas.symbol_width - 0.5 - delta, -0.5 + delta);
	warp->dst[1] = rt;

	warp->src[2] = POINT(warp->qrean->canvas.symbol_width - 0.5 - delta, warp->qrean->canvas.symbol_height - 0.5 - delta);
	warp->dst[2] = rb;

	if (lb.x != d.x || lb.y != d.y) {
		warp->src[3] = POINT(-0.5 + delta, warp->qrean->canvas.symbol_height - 0.5 - delta);
		warp->dst[3] = lb;
	}
	warp->h = create_image_transform_matrix(warp->src, warp->dst);

	int score = qrean_read_qr_corner_finder_pattern(warp->qrean, -1);
	if (score < 10) return 1;

	// restore if it doesn't fit
	*warp = backup;
	return 0;
}

int qrean_detector_try_decode_rmqr(image_t *src, qrean_detector_qr_finder_candidate_t *candidates, int num_candidates,
	void (*on_found)(qrean_detector_perspective_t *warp, void *opaque), void *opaque)
{
	int found = 0;
	for (int i = 0; i < num_candidates; i++) {
		for (int c = 0; c < 4; c++) {
			qrean_t *qrean = new_qrean(QREAN_CODE_TYPE_RMQR);
			qrean_set_qr_version(qrean, QR_VERSION_R17x139); // max size

			qrean_detector_perspective_t warp = create_qrean_detector_perspective(qrean, src);
			qrean_detector_perspective_setup_by_qr_finder_pattern_ring_corners(&warp, candidates[i].corners, c);

			if (!qrean_set_qr_format_info(qrean, qrean_read_qr_format_info(qrean, -1))) continue;
			qrean_debug_printf("Detected rMQR version: %s\n", qrspec_get_version_string(qrean->qr.version));

			qrean_detector_perspective_fit_for_rmqr(&warp);
			// if (qrean_read_qr_timing_pattern(qrean, -1) > 10) continue;

			if (qrean_fix_errors(qrean) >= 0) {
				on_found(&warp, opaque);
				found++;
			}
		}
	}

	return found;
}

int qrean_detector_try_decode_mqr(image_t *src, qrean_detector_qr_finder_candidate_t *candidates, int num_candidates,
	void (*on_found)(qrean_detector_perspective_t *warp, void *opaque), void *opaque)
{
	int found = 0;
	for (int i = 0; i < num_candidates; i++) {
		for (int c = 0; c < 4; c++) {
			qrean_t *qrean = new_qrean(QREAN_CODE_TYPE_MQR);
			qrean_set_qr_version(qrean, QR_VERSION_M4); // max size

			qrean_detector_perspective_t warp = create_qrean_detector_perspective(qrean, src);
			qrean_detector_perspective_setup_by_qr_finder_pattern_ring_corners(&warp, candidates[i].corners, c);

			if (!qrean_set_qr_format_info(qrean, qrean_read_qr_format_info(qrean, -1))) continue;
			if (qrean_read_qr_timing_pattern(qrean, -1) > 10) continue;

			if (qrean_fix_errors(qrean) >= 0) {
				on_found(&warp, opaque);
				found++;
			}
		}
	}

	return found;
}
