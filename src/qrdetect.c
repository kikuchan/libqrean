#include "image.h"
#include "qrdetect.h"
#include "runlength.h"

image_point_t *qrdetect_scan_finder_pattern(image_t *src, int *found) {
	static image_point_t candidates[MAX_CANDIDATES];
	int candidx = 0;

	image_t *img = image_clone(src);

	for (int y = 0; y < img->height; y++) {
		runlength_t rl = create_runlength();

		for (int x = 0; x < img->width; x++) {
			uint32_t v = image_read_pixel(img, POINT(x, y));
			if (v != 0 && v != PIXEL(255, 255, 255)) continue;
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
				if (!found_u && runlength_push_value(&rlvu, image_read_pixel(img, POINT(cx, cy - yy)))) {
					if (runlength_match_ratio(&rlvu, 4, 3, 2, 2, 0)) {
						found_u = runlength_get_count(&rl, 1);
					}
				}
				if (!found_d && runlength_push_value(&rlvd, image_read_pixel(img, POINT(cx, cy + yy)))) {
					if (runlength_match_ratio(&rlvd, 4, 3, 2, 2, 0)) {
						found_d = runlength_get_count(&rl, 1);
					}
				}
			}

			if (found_u && found_d) {
				// mark visit flag by painting inside as red
				image_extent_t r = image_paint(img, POINT(cx, cy), PIXEL(0, 255, 0));
				image_extent_dump(&r);

				cx = (r.left + r.right) / 2;
				cy = (r.top + r.bottom) / 2;

				if (candidx < MAX_CANDIDATES) {
					candidates[candidx++] = POINT(cx, cy);
				}
			}
		}
	}

	image_free(img);

	candidates[candidx] = POINT_INVALID;
	if (found) *found = candidx;

	return candidates;
}
