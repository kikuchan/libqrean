#include <stdio.h>

#include "qrmatrix.h"
#include "qrstream.h"
#include "qrdata.h"
#include "hexdump.h"

// return 1 to skip drawing
bit_t set_pixel(qrmatrix_t *qr, bitpos_t x, bitpos_t y, bitpos_t idx, bit_t v)
{
	return 0;
}

bit_t get_pixel(qrmatrix_t *qr, bitpos_t x, bitpos_t y, bitpos_t idx)
{
	return 1;
}

int main()
{
	qrmatrix_t qr = create_qrmatrix(QR_VERSION_5, QR_ERRORLEVEL_M, QR_MASKPATTERN_0);

//	qrmatrix_on_set_pixel(&qr, set_pixel);
//	qrmatrix_on_get_pixel(&qr, get_pixel);

	qrmatrix_write_string(&qr, "Hello, world");

	qrmatrix_dump(&qr, 1);

#if 0
	qrmatrix_set_pixel(&qr, qr.symbol_size - 1, qr.symbol_size - 1, 0);

	qrmatrix_dump(&qr, 1);

	qrmatrix_fix_errors(&qr);
	qrmatrix_dump(&qr, 1);
#endif
}
