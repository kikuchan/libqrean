#ifndef __QR_IMAGE_H__
#define __QR_IMAGE_H__

#include <stdint.h>

typedef uint32_t image_pixel_t;
#define PIXEL(r, g, b)     (((r) << 16) | ((g) << 8) | ((b) << 0))
#define PIXEL_GET_R(pix)   (((pix) >> 16) & 0xFF)
#define PIXEL_GET_G(pix)   (((pix) >> 8) & 0xFF)
#define PIXEL_GET_B(pix)   (((pix) >> 0) & 0xFF)

typedef uint32_t image_point_t;
#define POINT(x, y) ((uint32_t)((((uint16_t)(x)) << 16) | (((uint16_t)(y)) << 0)))
#define POINT_X(p) ((int16_t)((p) >> 16))
#define POINT_Y(p) ((int16_t)((p) >>  0))
#define POINT_INVALID (0x80008000)

typedef struct {
	image_pixel_t *buffer;

	uint32_t width;
	uint32_t height;
} image_t;

typedef struct {
	int top;
	int right;
	int bottom;
	int left;
} image_extent_t;

image_t *new_image(int width, int height);
void image_free(image_t *img);
image_t *image_clone(image_t *img);

void image_draw_pixel(image_t *img, image_point_t p, image_pixel_t pixel);
image_pixel_t image_read_pixel(image_t *img, image_point_t p);
void image_dump(image_t *img);
void image_draw_line(image_t *img, image_point_t s, image_point_t e, image_pixel_t pix, int thickness);
void image_draw_filled_rectangle(image_t *img, image_point_t s, int w, int h, image_pixel_t pix);
void image_draw_filled_ellipse(image_t *img, image_point_t center, int w, int h, image_pixel_t pix);
void image_draw_polygon(image_t *img, int N, image_point_t points[], image_pixel_t pixel, int thickness);

image_extent_t *image_extent_update(image_extent_t *extent, image_extent_t src);
void image_extent_dump(image_extent_t *extent);
image_extent_t image_paint(image_t *img, image_point_t p, image_pixel_t pix);

#endif
