#ifndef __QR_DETECT_H__
#define __QR_DETECT_H__

#include "image.h"
#include "qrmatrix.h"

#define MAX_CANDIDATES (10)

typedef struct {
	image_point_t center;
	image_extent_t extent;

	float center_x;
	float center_y;
	int area;
	float modsize_x;
	float modsize_y;
} qrdetector_candidate_t;

typedef struct {
	qrmatrix_t *qr;
	image_t *img;

	image_point_t src[4];
	image_point_t dst[4];
	image_transform_matrix_t h;
} qrdetector_perspective_t;

qrdetector_candidate_t *qrdetector_scan_finder_pattern(image_t *src, int *num_founds);

qrdetector_perspective_t create_qrdetector_perspective(qrmatrix_t *qr, image_t *img);
void qrdetector_perspective_setup_by_finder_pattern(qrdetector_perspective_t *warp, image_point_t src[3]);
int qrdetector_perspective_fit_by_alignment_pattern(qrdetector_perspective_t *d);

#endif /* __QR_DETECT_H__ */
