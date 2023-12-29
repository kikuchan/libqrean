#ifndef __QR_IMAGE_H__
#define __QR_IMAGE_H__

#include <stdint.h>
#include <stdio.h>

typedef uint32_t image_pixel_t;
#define PIXEL(r, g, b)   ((((uint32_t)(r)) << 16) | ((g) << 8) | ((b) << 0))
#define PIXEL_GET_R(pix) (((pix) >> 16) & 0xFF)
#define PIXEL_GET_G(pix) (((pix) >> 8) & 0xFF)
#define PIXEL_GET_B(pix) (((pix) >> 0) & 0xFF)

typedef struct {
	image_pixel_t *buffer;

	size_t width;
	size_t height;
} image_t;

typedef struct {
	int top;
	int right;
	int bottom;
	int left;
} image_extent_t;

typedef struct {
	image_extent_t extent;
	int area;
} image_paint_result_t;

typedef struct {
	/*
	 *  m0 m1 m2
	 *  m3 m4 m5
	 *  m6 m7  1
	 */
	float m[8];
} image_transform_matrix_t;

image_t *new_image(size_t width, size_t height);
void image_free(image_t *img);
image_t *image_clone(image_t *img);

// image_point_t
typedef struct {
	float x;
	float y;
} image_point_t;
#define POINT(x, y)         create_image_point((x), (y))
#define POINT_X(p)          ((p).x)
#define POINT_Y(p)          ((p).y)
#define POINT_INVALID       create_image_point(NAN, NAN)
#define POINT_IS_INVALID(p) (isnan((p).x) || isnan((p).y))

image_point_t create_image_point(float x, float y);
image_point_t image_point_add(image_point_t a, image_point_t b);
image_point_t image_point_sub(image_point_t a, image_point_t b);

void image_draw_pixel(image_t *img, image_point_t p, image_pixel_t pixel);
image_pixel_t image_read_pixel(image_t *img, image_point_t p);
void image_dump(image_t *img, FILE *out);
void image_save_as_ppm(image_t *img, FILE *out);
void image_draw_line(image_t *img, image_point_t s, image_point_t e, image_pixel_t pix, int thickness);
void image_draw_filled_rectangle(image_t *img, image_point_t s, int w, int h, image_pixel_t pix);
void image_draw_filled_ellipse(image_t *img, image_point_t center, int w, int h, image_pixel_t pix);
void image_draw_polygon(image_t *img, int N, image_point_t points[], image_pixel_t pixel, int thickness);

image_extent_t *image_extent_update(image_extent_t *extent, image_extent_t src);
image_point_t image_extent_center(image_extent_t *extent);

void image_extent_dump(image_extent_t *extent);
image_paint_result_t image_paint(image_t *img, image_point_t p, image_pixel_t pix);
float image_point_norm(image_point_t a);
float image_point_distance(image_point_t a, image_point_t b);
float image_point_angle(image_point_t base, image_point_t p);
void image_draw_extent(image_t *img, image_extent_t extent, image_pixel_t pix, int thickness);

image_point_t image_point_transform(image_point_t p, image_transform_matrix_t matrix);
image_transform_matrix_t create_image_transform_matrix(image_point_t src[4], image_point_t dst[4]);

void image_digitize(image_t *dst, image_t *src, float gamma_value);
void image_monochrome(image_t *dst, image_t *src, float gamma_value, int hist_result[256]);

#endif
