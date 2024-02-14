#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "image.h"
#include "qrean.h"

// #define RINT(x) rint((x))
// #define RINT(x) (x)
#define RINT(x) round(x)

void image_init(image_t *img, size_t width, size_t height, void *buffer)
{
	img->width = width;
	img->height = height;
	img->buffer = buffer;
}

image_pixel_t *new_image_buffer(size_t width, size_t height)
{
#ifndef NO_MALLOC
	return (image_pixel_t *)calloc(width * height, sizeof(image_pixel_t));
#else
	return NULL;
#endif
}

void image_buffer_free(image_pixel_t *buffer)
{
#ifndef NO_MALLOC
	free(buffer);
#endif
}

image_t *new_image(size_t width, size_t height)
{
#ifndef NO_MALLOC
	image_t *img = (image_t *)malloc(sizeof(image_t));
	if (!img) return NULL;

	void *buffer = new_image_buffer(width, height);
	if (!buffer) {
		free(img);
		return NULL;
	}
	image_init(img, width, height, buffer);

	return img;
#else
	return NULL;
#endif
}

void image_free(image_t *img)
{
#ifndef NO_MALLOC
	image_buffer_free(img->buffer);
	free(img);
#endif
}

image_t create_image(size_t width, size_t height, void *buffer)
{
	image_t img = {};
	image_init(&img, width, height, buffer);
	return img;
}

image_t *image_copy(image_t *dst, image_t *src)
{
	memcpy(dst->buffer, src->buffer, src->width * src->height * sizeof(image_pixel_t));
	return dst;
}

image_t *image_clone(image_t *img)
{
#ifndef NO_MALLOC
	image_t *clone = new_image(img->width, img->height);
	if (clone) image_copy(clone, img);
	return clone;
#else
	return NULL;
#endif
}

image_point_t create_image_point(float x, float y)
{
	image_point_t p = { .x = x, .y = y };
	return p;
}

image_point_t image_point_add(image_point_t a, image_point_t b)
{
	return POINT(POINT_X(a) + POINT_X(b), POINT_Y(a) + POINT_Y(b));
}
image_point_t image_point_sub(image_point_t a, image_point_t b)
{
	return POINT(POINT_X(a) - POINT_X(b), POINT_Y(a) - POINT_Y(b));
}

void image_draw_pixel(image_t *img, image_point_t p, image_pixel_t pixel)
{
	int x = RINT(POINT_X(p));
	int y = RINT(POINT_Y(p));
	if (x < 0 || y < 0 || x >= (int)img->width || y >= (int)img->height) return;
	img->buffer[y * img->width + x] = pixel | ~PIXEL_MASK;
}

image_pixel_t image_read_pixel(image_t *img, image_point_t p)
{
	int x = RINT(POINT_X(p));
	int y = RINT(POINT_Y(p));
	if (x < 0 || y < 0 || x >= (int)img->width || y >= (int)img->height) return 0;
	return img->buffer[y * img->width + x] & PIXEL_MASK;
}

void image_save_as_ppm(image_t *img, FILE *out)
{
#ifndef NO_PRINTF
	fprintf(out, "P6\n%zu %zu\n255\n", img->width, img->height);

	for (uint32_t y = 0; y < img->height; y++) {
		for (uint32_t x = 0; x < img->width; x++) {
			image_pixel_t pix = image_read_pixel(img, POINT(x, y));
			fputc(PIXEL_GET_R(pix), out);
			fputc(PIXEL_GET_G(pix), out);
			fputc(PIXEL_GET_B(pix), out);
		}
	}
#endif
}

void image_dump(image_t *img, FILE *out)
{
	image_save_as_ppm(img, out);
}

void image_draw_filled_rectangle(image_t *img, image_point_t p, int w, int h, image_pixel_t pix)
{
	int x = RINT(POINT_X(p));
	int y = RINT(POINT_Y(p));
	int ix, iy;

	for (iy = 0; iy < h; iy++) {
		for (ix = 0; ix < w; ix++) {
			image_draw_pixel(img, POINT(x + ix, y + iy), pix);
		}
	}
}

static void swap(int *a, int *b)
{
	int tmp;

	tmp = *a;
	*a = *b;
	*b = tmp;
}

void image_draw_line(image_t *img, image_point_t s, image_point_t e, image_pixel_t pix, int thickness)
{
	int x0 = RINT(POINT_X(s));
	int y0 = RINT(POINT_Y(s));
	int x1 = RINT(POINT_X(e));
	int y1 = RINT(POINT_Y(e));
	int steep = abs(y1 - y0) > abs(x1 - x0);
	int deltax;
	int deltay;
	int error = 0;
	int x, y;
	int ystep;

	if (steep) {
		swap(&x0, &y0);
		swap(&x1, &y1);
	}
	if (x0 > x1) {
		swap(&x0, &x1);
		swap(&y0, &y1);
	}

	deltax = x1 - x0;
	deltay = abs(y1 - y0);
	y = y0;

	ystep = (y0 < y1) ? 1 : -1;

	for (x = x0; x <= x1; x++) {
		if (steep) {
			if (thickness > 0) {
				image_draw_filled_rectangle(img, POINT(y, x), thickness, 1, pix);
			} else {
				image_draw_pixel(img, POINT(y, x), pix);
			}
		} else {
			if (thickness > 0) {
				image_draw_filled_rectangle(img, POINT(x, y), 1, thickness, pix);
			} else {
				image_draw_pixel(img, POINT(x, y), pix);
			}
		}

		error += deltay;
		if (2 * error >= deltax) {
			y += ystep;
			error -= deltax;
		}
	}
}

void image_draw_polygon(image_t *img, int N, image_point_t points[], image_pixel_t pixel, int thickness)
{
	for (int i = 0; i < N; i++) {
		image_draw_line(img, points[i], points[(i + 1) % N], pixel, thickness);
	}
}

void image_draw_filled_ellipse(image_t *img, image_point_t center, int w, int h, image_pixel_t pix)
{
	float cx = POINT_X(center);
	float cy = POINT_Y(center);
	int x, y;

	float sx = cx - w;
	float sy = cy - h;
	float dx = cx + w;
	float dy = cy + h;

	for (y = sy; y <= dy; y++) {
		for (x = sx; x <= dx; x++) {
			float tx, ty;

			tx = x - cx;
			ty = y - cy;

			if (((tx * tx) / (w * w) + (ty * ty) / (h * h)) <= 1.0f) {
				image_draw_pixel(img, POINT(x, y), pix);
			}
		}
	}
}

image_extent_t create_image_extent()
{
	image_extent_t e = { NAN, NAN, NAN, NAN };
	return e;
}

image_extent_t *image_extent_update(image_extent_t *extent, image_extent_t src)
{
	if (isnan(extent->top) || src.top < extent->top) extent->top = src.top;
	if (isnan(extent->left) || src.left < extent->left) extent->left = src.left;
	if (isnan(extent->right) || extent->right < src.right) extent->right = src.right;
	if (isnan(extent->bottom) || extent->bottom < src.bottom) extent->bottom = src.bottom;
	return extent;
}

image_point_t image_extent_center(image_extent_t *extent)
{
	return POINT((extent->left + extent->right) / 2.0, (extent->top + extent->bottom) / 2.0);
}

void image_extent_dump(image_extent_t *extent)
{
	fprintf(stderr, "[%f %f %f %f]\n", extent->top, extent->right, extent->bottom, extent->left);
}

static void image_paint_update_result(image_paint_result_t *target, image_paint_result_t result)
{
	if (result.area) {
		image_extent_update(&target->extent, result.extent);
		target->area += result.area;
	}
}

image_paint_result_t image_paint(image_t *img, image_point_t center, image_pixel_t pix)
{
	float cx = POINT_X(center);
	float cy = POINT_Y(center);
	image_paint_result_t result = {
		{cy, cx, cy, cx},
		0,
	};

	if (cx < 0 || cy < 0 || cx >= (int)img->width || cy >= (int)img->height) return result;

	image_pixel_t bg = image_read_pixel(img, center);
	if (bg == pix) return result;

	int lx, rx;
	for (lx = cx; lx >= 0 && image_read_pixel(img, POINT(lx, cy)) == bg; lx--) {
		image_draw_pixel(img, POINT(lx, cy), pix);
		result.area++;
		result.extent.left = lx;
	}
	for (rx = cx + 1; rx < (int)img->width && image_read_pixel(img, POINT(rx, cy)) == bg; rx++) {
		image_draw_pixel(img, POINT(rx, cy), pix);
		result.area++;
		result.extent.right = rx;
	}

	for (int x = lx; x < rx; x++) {
		if (image_read_pixel(img, POINT(x, cy - 1)) == bg) image_paint_update_result(&result, image_paint(img, POINT(x, cy - 1), pix));
		if (image_read_pixel(img, POINT(x, cy + 1)) == bg) image_paint_update_result(&result, image_paint(img, POINT(x, cy + 1), pix));
	}

	return result;
}

float image_point_norm(image_point_t a)
{
	return sqrt((POINT_X(a) * POINT_X(a)) + (POINT_Y(a) * POINT_Y(a)));
}

float image_point_distance(image_point_t a, image_point_t b)
{
	return image_point_norm(image_point_sub(a, b));
	// return sqrt((POINT_X(a) - POINT_X(b)) * (POINT_X(a) - POINT_X(b)) + (POINT_Y(a) - POINT_Y(b)) * (POINT_Y(a) - POINT_Y(b)));
}

float image_point_angle(image_point_t base, image_point_t p)
{
	return atan2f(p.y - base.y, p.x - base.x);
}

void image_draw_extent(image_t *img, image_extent_t extent, image_pixel_t pix, int thickness)
{
	image_point_t points[] = {
		POINT(extent.left, extent.top),
		POINT(extent.right, extent.top),
		POINT(extent.right, extent.bottom),
		POINT(extent.left, extent.bottom),
	};

	image_draw_polygon(img, 4, points, pix, thickness);
}

void image_monochrome(image_t *dst, image_t *src, float gamma_value, size_t hist_result[256])
{
	size_t hist[256] = { 0 };
	for (int y = 0; y < (int)src->height; y++) {
		for (int x = 0; x < (int)src->width; x++) {
			image_pixel_t pix = image_read_pixel(src, POINT(x, y));

			uint8_t value
				= pow((0.299 * PIXEL_GET_R(pix) + 0.587 * PIXEL_GET_G(pix) + 0.114 * PIXEL_GET_B(pix)) / 255.0, 1 / gamma_value) * 255;
			image_draw_pixel(dst, POINT(x, y), PIXEL(value, value, value));
			hist[value]++;
		}
	}
	if (hist_result) memcpy(hist_result, hist, sizeof(hist));
}

void image_digitize(image_t *dst, image_t *src, float gamma_value)
{
	// Using otsu method
	// https://en.wikipedia.org/wiki/Otsu%27s_method
	size_t hist[256] = { 0 };
	uint8_t threshold = 0;
	float max_sigma = 0.0;

	image_monochrome(dst, src, gamma_value, hist);

	for (int t = 0; t < 256; t++) {
		float w0 = 0.0;
		float w1 = 0.0;
		float m0 = 0.0;
		float m1 = 0.0;
		float sigma = 0.0;

		for (int i = 0; i < 256; i++) {
			if (i < t) {
				w0 += hist[i];
				m0 += i * hist[i];
			} else {
				w1 += hist[i];
				m1 += i * hist[i];
			}
		}

		if (w0 == 0.0 || w1 == 0.0) continue;

		m0 /= w0;
		m1 /= w1;

		sigma = (w0 * w1) * ((m0 - m1) * (m0 - m1));

		if (sigma > max_sigma) {
			max_sigma = sigma;
			threshold = t;
		}
	}
	qrean_debug_printf("threshold: %d\n", threshold);

	for (int y = 0; y < (int)dst->height; y++) {
		for (int x = 0; x < (int)dst->width; x++) {
			image_pixel_t pix = image_read_pixel(dst, POINT(x, y));
			image_draw_pixel(dst, POINT(x, y), PIXEL_GET_R(pix) < threshold ? PIXEL(0, 0, 0) : PIXEL(255, 255, 255));
		}
	}
}

image_point_t image_point_transform(image_point_t p, image_transform_matrix_t matrix)
{
	float x = matrix.m[0] * POINT_X(p) + matrix.m[1] * POINT_Y(p) + matrix.m[2];
	float y = matrix.m[3] * POINT_X(p) + matrix.m[4] * POINT_Y(p) + matrix.m[5];
	float w = matrix.m[6] * POINT_X(p) + matrix.m[7] * POINT_Y(p) + 1.0;

	return POINT((x / w), (y / w));
}

image_transform_matrix_t create_image_transform_matrix(image_point_t src[4], image_point_t dst[4])
{
	float src_x0 = POINT_X(src[0]);
	float src_y0 = POINT_Y(src[0]);
	float src_x1 = POINT_X(src[1]);
	float src_y1 = POINT_Y(src[1]);
	float src_x2 = POINT_X(src[2]);
	float src_y2 = POINT_Y(src[2]);
	float src_x3 = POINT_X(src[3]);
	float src_y3 = POINT_Y(src[3]);

	float dst_x0 = POINT_X(dst[0]);
	float dst_y0 = POINT_Y(dst[0]);
	float dst_x1 = POINT_X(dst[1]);
	float dst_y1 = POINT_Y(dst[1]);
	float dst_x2 = POINT_X(dst[2]);
	float dst_y2 = POINT_Y(dst[2]);
	float dst_x3 = POINT_X(dst[3]);
	float dst_y3 = POINT_Y(dst[3]);

	float a[8][8] = {
		{src_x0, src_y0, 1,      0,      0, 0, -src_x0 * dst_x0, -src_y0 * dst_x0},
		{     0,      0, 0, src_x0, src_y0, 1, -src_x0 * dst_y0, -src_y0 * dst_y0},
		{src_x1, src_y1, 1,      0,      0, 0, -src_x1 * dst_x1, -src_y1 * dst_x1},
		{     0,      0, 0, src_x1, src_y1, 1, -src_x1 * dst_y1, -src_y1 * dst_y1},
		{src_x2, src_y2, 1,      0,      0, 0, -src_x2 * dst_x2, -src_y2 * dst_x2},
		{     0,      0, 0, src_x2, src_y2, 1, -src_x2 * dst_y2, -src_y2 * dst_y2},
		{src_x3, src_y3, 1,      0,      0, 0, -src_x3 * dst_x3, -src_y3 * dst_x3},
		{     0,      0, 0, src_x3, src_y3, 1, -src_x3 * dst_y3, -src_y3 * dst_y3},
	};
	float b[8] = {
		dst_x0,
		dst_y0,
		dst_x1,
		dst_y1,
		dst_x2,
		dst_y2,
		dst_x3,
		dst_y3,
	};

	// Gauss-Jordan elimination
	for (int i = 0; i < 8; i++) {
		int pivot = i;
		for (int j = i + 1; j < 8; j++) {
			if (fabs(a[j][i]) > fabs(a[pivot][i])) {
				pivot = j;
			}
		}

		for (int j = 0; j < 8; j++) {
			float tmp = a[i][j];
			a[i][j] = a[pivot][j];
			a[pivot][j] = tmp;
		}
		{
			float tmp = b[i];
			b[i] = b[pivot];
			b[pivot] = tmp;
		}

		for (int j = i + 1; j < 8; j++) {
			float ratio = a[j][i] / a[i][i];
			for (int k = i; k < 8; k++) {
				a[j][k] -= a[i][k] * ratio;
			}
			b[j] -= b[i] * ratio;
		}
	}

	for (int i = 7; i >= 0; i--) {
		for (int j = i + 1; j < 8; j++) {
			b[i] -= a[i][j] * b[j];
		}
		b[i] /= a[i][i];
	}

	image_transform_matrix_t matrix = {
		.m = {b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7]},
	};

	return matrix;
}

void image_morphology_erode(image_t *dst)
{
	CREATE_IMAGE_BY_CLONE(src, dst);
	for (int y = 0; y < (int)src.height; y++) {
		for (int x = 0; x < (int)src.width; x++) {
			image_pixel_t pix = image_read_pixel(&src, POINT(x, y));
			if (pix == 0) continue;

			for (int dy = -1; dy <= 1; dy++) {
				for (int dx = -1; dx <= 1; dx++) {
					if (dx == 0 && dy == 0) continue;
					if (x + dx < 0 || y + dy < 0 || x + dx >= (int)src.width || y + dy >= (int)src.height) continue;
					if (image_read_pixel(&src, POINT(x + dx, y + dy)) == 0) {
						image_draw_pixel(dst, POINT(x, y), 0);
						goto next;
					}
				}
			}
		next:;
		}
	}
	DESTROY_IMAGE(src);
}

void image_morphology_dilate(image_t *dst)
{
	CREATE_IMAGE_BY_CLONE(src, dst);
	for (int y = 0; y < (int)src.height; y++) {
		for (int x = 0; x < (int)src.width; x++) {
			image_pixel_t pix = image_read_pixel(&src, POINT(x, y));
			if (pix != 0) continue;

			for (int dy = -1; dy <= 1; dy++) {
				for (int dx = -1; dx <= 1; dx++) {
					if (dx == 0 && dy == 0) continue;
					if (x + dx < 0 || y + dy < 0 || x + dx >= (int)src.width || y + dy >= (int)src.height) continue;
					if (image_read_pixel(&src, POINT(x + dx, y + dy)) != 0) {
						image_draw_pixel(dst, POINT(x, y), PIXEL(255, 255, 255));
						goto next;
					}
				}
			}
		next:;
		}
	}
	DESTROY_IMAGE(src);
}

void image_morphology_close(image_t *dst)
{
	image_morphology_erode(dst);
	image_morphology_dilate(dst);
}

void image_morphology_open(image_t *dst)
{
	image_morphology_dilate(dst);
	image_morphology_erode(dst);
}
