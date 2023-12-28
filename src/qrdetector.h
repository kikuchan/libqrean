#ifndef __QR_DETECT_H__
#define __QR_DETECT_H__

#include "image.h"
#include "qrean.h"

#define MAX_CANDIDATES (10)

typedef struct {
	image_point_t center;
	image_extent_t extent;

	image_point_t corners[4];

	int area;
} qrdetector_finder_candidate_t;

typedef struct {
	qrean_t *qrean;
	image_t *img;

	image_point_t src[4];
	image_point_t dst[4];
	image_transform_matrix_t h;
} qrdetector_perspective_t;

qrdetector_finder_candidate_t *qrdetector_scan_finder_pattern(image_t *src, int *num_founds);

qrdetector_perspective_t create_qrdetector_perspective(qrean_t *qrean, image_t *img);

void qrdetector_perspective_setup_by_finder_pattern_qr(qrdetector_perspective_t *warp, image_point_t src[3]);
void qrdetector_perspective_setup_by_finder_pattern_ring_corners(qrdetector_perspective_t *warp, image_point_t ring[4], int offset);

int qrdetector_perspective_fit_for_qr(qrdetector_perspective_t *warp);
int qrdetector_perspective_fit_for_rmqr(qrdetector_perspective_t *warp);

int qrdetector_try_decode_qr(image_t *src, qrdetector_finder_candidate_t *candidates, int num_candidates,
	void (*on_found)(qrdetector_perspective_t *warp, void *opaque), void *opaque);
int qrdetector_try_decode_mqr(image_t *src, qrdetector_finder_candidate_t *candidates, int num_candidates,
	void (*on_found)(qrdetector_perspective_t *warp, void *opaque), void *opaque);
int qrdetector_try_decode_rmqr(image_t *src, qrdetector_finder_candidate_t *candidates, int num_candidates,
	void (*on_found)(qrdetector_perspective_t *warp, void *opaque), void *opaque);

#endif /* __QR_DETECT_H__ */
