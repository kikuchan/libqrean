#include <stdio.h>

#include "qrmatrix.h"
#include "qrtypes.h"
#include "debug.h"
#include "qrspec.h"
#include "image.h"

int main() {
	qrean_on_debug_vprintf(vfprintf, stderr);

	qrmatrix_t qr = create_qrmatrix();
	qrmatrix_set_version(&qr, QR_VERSION_AUTO);

	qrmatrix_write_string_8bit(&qr, "https://github.com/kikuchan/libqrean");

#if 0
	qrmatrix_dump(&qr, stderr);
#else
	size_t width = qr.symbol_width + qr.padding.l + qr.padding.r;
	size_t height = qr.symbol_height + qr.padding.t + qr.padding.b;
	image_t *img = new_image(width, height);
	qrmatrix_read_bitmap(&qr, img->buffer, width * height * 4, 32);

	image_dump(img, stdout);
#endif
}
