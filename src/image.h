#ifndef __QR_IMAGE_H__
#define __QR_IMAGE_H__

#include <stdint.h>

typedef struct {
	uint32_t *buffer;

	uint32_t width;
	uint32_t height;
} image_t;

typedef struct {
	int top;
	int right;
	int bottom;
	int left;
} image_extent_t;

#define MAKE_RGB(r, g, b) (((r) << 16) | ((g) << 8) | ((b) << 0))
#define PIXEL_R(pix)      (((pix) >> 16) & 0xFF)
#define PIXEL_G(pix)      (((pix) >> 8) & 0xFF)
#define PIXEL_B(pix)      (((pix) >> 0) & 0xFF)

image_t *new_image(int width, int height);
void image_write_pixel(image_t *img, int x, int y, uint32_t pixel);
uint32_t image_read_pixel(image_t *img, int x, int y);
void image_dump(image_t *img);
void image_write_line(image_t *img, int x0, int y0, int x1, int y1, uint32_t pix, int thickness);
void image_write_filled_rectangle(image_t *img, int x, int y, int w, int h, uint32_t pix);
void image_write_filled_ellipse(image_t *img, int cx, int cy, int w, int h, uint32_t pix);

image_extent_t *image_extent_update(image_extent_t *extent, image_extent_t src);
void image_extent_dump(image_extent_t *extent);
image_extent_t image_paint(image_t *img, int cx, int cy, uint32_t pix);

#endif
