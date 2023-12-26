#include <stdio.h>

#include "qrean.h"
#include "debug.h"
#include "image.h"

int main() {
	qrean_on_debug_vprintf(vfprintf, stderr);

	qrean_t qrean = create_qrean(QREAN_CODE_TYPE_RMQR);
	qrean_write_string(&qrean, "https://github.com/kikuchan/libqrean", QREAN_DATA_TYPE_AUTO);

#if 0
	qrean_dump(&qr, stderr);
#else
	size_t width = qrean_get_bitmap_width(&qrean);
	size_t height = qrean_get_bitmap_height(&qrean);
	image_t *img = new_image(width, height);
	qrean_read_bitmap(&qrean, img->buffer, width * height * 4, 32);

	image_dump(img, stdout);
#endif
}
