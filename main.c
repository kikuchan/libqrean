#include <stdio.h>

#include "hexdump.h"
#include "qrdata.h"
#include "qrmatrix.h"
#include "qrstream.h"

#include "barcode.h"

// return 1 to skip drawing
bit_t put_pixel(qrmatrix_t *qr, bitpos_t x, bitpos_t y, bitpos_t idx, bit_t v) {
	/*
	    char buffer[8192];
	    size_t len = qrmatrix_get_bitmap(qr, buffer, sizeof(buffer), 8);

	    printf("P5\n%d %d\n255\n", qr->symbol_size, qr->symbol_size);
	    fwrite(buffer, 1, len, stdout);
	*/
	return 0;
}

bit_t get_pixel(qrmatrix_t *qr, bitpos_t x, bitpos_t y, bitpos_t idx) {
	return 1;
}

int main() {
	qrmatrix_t qr = create_qrmatrix(QR_VERSION_5, QR_ERRORLEVEL_M, QR_MASKPATTERN_0);

	qrmatrix_on_put_pixel(&qr, put_pixel);
	//	qrmatrix_on_get_pixel(&qr, get_pixel);

	qrmatrix_put_string(&qr, "Hello, world");

	qrmatrix_dump(&qr, 1);

	barcode_t code = create_barcode(BARCODE_TYPE_EAN13);
	if (barcode_put_string(&code, "123456789012")) {
		barcode_dump(&code, 10);
	}

#if 0
	qrmatrix_put_pixel(&qr, qr.symbol_size - 1, qr.symbol_size - 1, 0);

	qrmatrix_dump(&qr, 1);

	qrmatrix_fix_errors(&qr);
	qrmatrix_dump(&qr, 1);
#endif
}
