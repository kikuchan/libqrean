#include <stdio.h>
#include <stdarg.h>

#include "qrdata.h"
#include "qrean.h"
#include "image.h"
#include "detector.h"
#include "debug.h"
#include "printf.h"

__attribute__((import_module("env"), import_name("debug")))
extern void debug(const char *str);
void _putchar(char ch) {}

static int debug_vfprintf(void *opaque, const char *fmt, va_list ap) {
	char buf[256];
	vsnprintf(buf, sizeof(buf), fmt, ap);
	debug(buf);
	return 0;
}

void enable_debug() {
	qrean_on_debug_vprintf(debug_vfprintf, NULL);
	qrean_debug_printf("DEBUG enabled\n");
}

typedef struct {
	int codetype;
	int datatype;
	int qr_errorlevel;
	int qr_version;
	int qr_maskpattern;
	int eci_code;
	int padding[4];
	int scale;
} encode_options_t;

image_t* encode(const char *inputbuf, encode_options_t *opts) {
	qrean_t qrean = create_qrean(opts->codetype);
	if (!qrean_is_valid(&qrean)) {
		return 0;
	}

	qrean_set_qr_version(&qrean, opts->qr_version);
	qrean_set_qr_maskpattern(&qrean, opts->qr_maskpattern);
	qrean_set_qr_errorlevel(&qrean, opts->qr_errorlevel);
	qrean_set_eci_code(&qrean, opts->eci_code);

	padding_t p = qrean_get_bitmap_padding(&qrean);
	for (int i = 0; i < 4; i++) {
		if (opts->padding[i] >= 0) p.padding[i] = opts->padding[i];
	}
	qrean_set_bitmap_padding(&qrean, p);
	qrean_set_bitmap_scale(&qrean, opts->scale);

	int res = qrean_write_string(&qrean, inputbuf, opts->datatype);
	if (!res) {
		return 0;
	}

	size_t width = qrean_get_bitmap_width(&qrean);
	size_t height = qrean_get_bitmap_height(&qrean);

	image_t *img = malloc(width * height * sizeof(image_pixel_t));
	if (img) {
		img->width = width;
		img->height = height;
		img->buffer = (image_pixel_t*)(((char*)img) + sizeof(image_t));

		qrean_set_bitmap_color(&qrean, 0x000000FF, 0xFFFFFFFF);
		qrean_read_bitmap(&qrean, img->buffer, width * height * 4, 32);
	}
	return img;
}

__attribute__((import_module("env"), import_name("on_found")))
extern void on_found_js(int type, const char *str, int version, int level, int mask, image_point_t *points);

typedef struct {
	char *outputbuf;
	unsigned int size;
	int eci_code;
} on_found_params_t;

static void on_found(qrean_detector_perspective_t *warp, void *opaque) {
	on_found_params_t *params = (on_found_params_t *)opaque;

	qrean_set_eci_code(warp->qrean, params->eci_code);
	qrean_read_string(warp->qrean, params->outputbuf, params->size);

	image_point_t points[] = {
		image_point_transform(POINT(-0.5, -0.5), warp->h),
		image_point_transform(POINT(warp->qrean->canvas.symbol_width - 0.5, -0.5), warp->h),
		image_point_transform(POINT(warp->qrean->canvas.symbol_width - 0.5, warp->qrean->canvas.symbol_height - 0.5), warp->h),
		image_point_transform(POINT(- 0.5, warp->qrean->canvas.symbol_height - 0.5), warp->h),
	};

	on_found_js(
		  warp->qrean->code->type
		, params->outputbuf
		, warp->qrean->qr.version
		, warp->qrean->qr.level
		, warp->qrean->qr.mask
		, points
	);
}

int detect(char *outputbuf, unsigned int size, image_t *img, double gamma_value, int eci_code) {
	outputbuf[0] = '\0';

	// overwrite. it's safe with src == dst
	image_digitize(img, img, gamma_value);
	int found = 0;

	int num_candidates = 0;
	qrean_detector_qr_finder_candidate_t* candidates = qrean_detector_scan_qr_finder_pattern(img, &num_candidates);

	on_found_params_t params = {
		outputbuf,
		size,
		eci_code,
	};

	found += qrean_detector_try_decode_qr(img, candidates, num_candidates, on_found, &params);
	found += qrean_detector_try_decode_mqr(img, candidates, num_candidates, on_found, &params);
	found += qrean_detector_try_decode_rmqr(img, candidates, num_candidates, on_found, &params);
	found += qrean_detector_try_decode_tqr(img, candidates, num_candidates, on_found, &params);
	found += qrean_detector_scan_barcodes(img, on_found, &params);

	return found;
}
