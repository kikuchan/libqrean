#ifndef __QREAN_DETECTOR_H__
#define __QREAN_DETECTOR_H__

#include "image.h"
#include "qrean.h"

#define MAX_CANDIDATES (30)

typedef struct {
	image_point_t center;
	image_extent_t extent;

	image_point_t corners[4];

	int area;
} qrean_detector_qr_finder_candidate_t;

typedef struct {
	uint_fast16_t code;
	image_point_t s;
	float barsize;
} qrean_detector_barcode_candidate_t;

typedef struct {
	qrean_t *qrean;
	image_t *img;

	image_point_t src[4];
	image_point_t dst[4];
	image_transform_matrix_t h;
} qrean_detector_perspective_t;

int qrean_detector_scan_barcodes(image_t *src, void (*on_found)(qrean_detector_perspective_t *warp, void *opaque), void *opaque);
qrean_detector_qr_finder_candidate_t *qrean_detector_scan_qr_finder_pattern(image_t *src, int *num_founds);

qrean_detector_perspective_t create_qrean_detector_perspective(qrean_t *qrean, image_t *img);

bit_t qrean_detector_perspective_read_image_pixel(qrean_t *qrean, bitpos_t x, bitpos_t y, bitpos_t pos, void *opaque);

void qrean_detector_perspective_setup_by_qr_finder_pattern_centers(
	qrean_detector_perspective_t *warp, image_point_t src[3], int border_offset);
void qrean_detector_perspective_setup_by_qr_finder_pattern_ring_corners(
	qrean_detector_perspective_t *warp, image_point_t ring[4], int offset);

int qrean_detector_perspective_fit_for_qr(qrean_detector_perspective_t *warp);
int qrean_detector_perspective_fit_for_rmqr(qrean_detector_perspective_t *warp);

int qrean_detector_try_decode_qr(image_t *src, qrean_detector_qr_finder_candidate_t *candidates, int num_candidates,
	void (*on_found)(qrean_detector_perspective_t *warp, void *opaque), void *opaque);
int qrean_detector_try_decode_mqr(image_t *src, qrean_detector_qr_finder_candidate_t *candidates, int num_candidates,
	void (*on_found)(qrean_detector_perspective_t *warp, void *opaque), void *opaque);
int qrean_detector_try_decode_rmqr(image_t *src, qrean_detector_qr_finder_candidate_t *candidates, int num_candidates,
	void (*on_found)(qrean_detector_perspective_t *warp, void *opaque), void *opaque);
int qrean_detector_try_decode_tqr(image_t *src, qrean_detector_qr_finder_candidate_t *candidates, int num_candidates,
	void (*on_found)(qrean_detector_perspective_t *warp, void *opaque), void *opaque);

#endif /* __QREAN_DETECTOR_H__ */
