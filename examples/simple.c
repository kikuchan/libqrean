#include <stdio.h>

#include "qrmatrix.h"
#include "qrtypes.h"
#include "debug.h"
#include "qrspec.h"
#include "hexdump.h"

int main() {
	qrean_on_debug_printf(vprintf);

	qrmatrix_t qr = create_qrmatrix();
	qrmatrix_set_version(&qr, QR_VERSION_M4);
	qrmatrix_set_errorlevel(&qr, QR_ERRORLEVEL_M);
	qrmatrix_set_maskpattern(&qr, QR_MASKPATTERN_0);

//	qrmatrix_write_function_patterns(&qr);
	qrmatrix_write_string_8bit(&qr, "Hello");

	qrpayload_t *payload = new_qrpayload(qr.version, qr.level);
	qrmatrix_read_payload(&qr, payload);

	hexdump(payload->buffer, payload->total_words, 0);

	fprintf(stderr, "total words: %ld / %ld\n", payload->total_words, payload->data_words);
	qrmatrix_dump(&qr, stderr);

	qrmatrix_fix_errors(&qr);
	qrmatrix_dump(&qr, stderr);
}
