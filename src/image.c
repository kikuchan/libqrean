#include "image.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

image_t *new_image(int width, int height) {
	image_t *img = (image_t *)malloc(sizeof(image_t));
	if (!img) return NULL;

	img->width = width;
	img->height = height;
	img->buffer = (uint32_t *)calloc(width * height, sizeof(uint32_t));
	if (!img->buffer) {
		free(img);
		return NULL;
	}

	return img;
}

void image_free(image_t *img)
{
	free(img->buffer);
	free(img);
}

image_t *image_clone(image_t *img)
{
	image_t *clone = new_image(img->width, img->height);
	if (clone) {
		memcpy(clone->buffer, img->buffer, img->width * img->height * sizeof(uint32_t));
	}
	return clone;
}

void image_write_pixel(image_t *img, int x, int y, uint32_t pixel) {
	if (x < 0 || y < 0 || x >= (int)img->width || y >= (int)img->height) return;
	img->buffer[y * img->width + x] = pixel;
}

uint32_t image_read_pixel(image_t *img, int x, int y) {
	if (x < 0 || y < 0 || x >= (int)img->width || y >= (int)img->height) return 0;
	return img->buffer[y * img->width + x];
}

void image_dump(image_t *img) {
	printf("P6\n%u %u\n255\n", img->width, img->height);

	for (uint32_t y = 0; y < img->height; y++) {
		for (uint32_t x = 0; x < img->width; x++) {
			uint32_t pix = image_read_pixel(img, x, y);
			putchar(PIXEL_R(pix));
			putchar(PIXEL_G(pix));
			putchar(PIXEL_B(pix));
		}
	}
}

void image_write_filled_rectangle(image_t *img, int x, int y, int w, int h, uint32_t pix) {
	int ix, iy;

	for (iy = 0; iy < h; iy++) {
		for (ix = 0; ix < w; ix++) {
			image_write_pixel(img, x + ix, y + iy, pix);
		}
	}
}

static void swap(int *a, int *b) {
	int tmp;

	tmp = *a;
	*a = *b;
	*b = tmp;
}

void image_write_line(image_t *img, int x0, int y0, int x1, int y1, uint32_t pix, int thickness) {
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
				image_write_filled_rectangle(img, y, x, thickness, 1, pix);
			} else {
				image_write_pixel(img, y, x, pix);
			}
		} else {
			if (thickness > 0) {
				image_write_filled_rectangle(img, x, y, thickness, 1, pix);
			} else {
				image_write_pixel(img, x, y, pix);
			}
		}

		error += deltay;
		if (2 * error >= deltax) {
			y += ystep;
			error -= deltax;
		}
	}
}

void image_write_filled_ellipse(image_t *img, int cx, int cy, int w, int h, uint32_t pix) {
	float a, b;
	int x, y;

	a = w / 2.0f;
	b = h / 2.0f;

	float sx = cx - a;
	float sy = cy - b;
	float dx = cx + a;
	float dy = cy + b;

	for (y = sy; y <= dy; y++) {
		for (x = sx; x <= dx; x++) {
			float tx, ty;

			tx = x - cx + 0.5f;
			ty = y - cy + 0.5f;

			if (((tx * tx) / (a * a) + (ty * ty) / (b * b)) < 1.0f) {
				image_write_pixel(img, x, y, pix);
			}
		}
	}
}

image_extent_t *image_extent_update(image_extent_t *extent, image_extent_t src) {
	if (src.top < extent->top) extent->top = src.top;
	if (src.left < extent->left) extent->left = src.left;
	if (extent->right < src.right) extent->right = src.right;
	if (extent->bottom < src.bottom) extent->bottom = src.bottom;
	return extent;
}

void image_extent_dump(image_extent_t *extent) {
	fprintf(stderr, "[%u %u %u %u]\n", extent->top, extent->right, extent->bottom, extent->left);
}

image_extent_t image_paint(image_t *img, int cx, int cy, uint32_t pix) {
	image_extent_t extent = {cy, cx, cy, cx};

	uint32_t bg = image_read_pixel(img, cx, cy);
	if (bg == pix) return extent;

	image_write_pixel(img, cx, cy, pix);

	if (image_read_pixel(img, cx + 1, cy) == bg) image_extent_update(&extent, image_paint(img, cx + 1, cy, pix));
	if (image_read_pixel(img, cx - 1, cy) == bg) image_extent_update(&extent, image_paint(img, cx - 1, cy, pix));
	if (image_read_pixel(img, cx, cy + 1) == bg) image_extent_update(&extent, image_paint(img, cx, cy + 1, pix));
	if (image_read_pixel(img, cx, cy - 1) == bg) image_extent_update(&extent, image_paint(img, cx, cy - 1, pix));

	return extent;
}

