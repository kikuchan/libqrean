#include <stdio.h>

#include "qrean.h"
#include "debug.h"
#include "image.h"

int main() {
	qrean_on_debug_vprintf(vfprintf, stderr);

	qrean_t qrean = create_qrean(QREAN_CODE_TYPE_QR);
	qrean_write_string(&qrean, "https://github.com/kikuchan/libqrean", QREAN_DATA_TYPE_AUTO);

	// inject noise at the bottom right
	qrean_write_pixel(&qrean, qrean.canvas.symbol_width - 1, qrean.canvas.symbol_height - 1, 1);

	// fix errors
	qrean_fix_errors(&qrean);

	char buffer[1024];
	qrean_read_string(&qrean, buffer, sizeof(buffer));
	printf("%s\n", buffer);
}
