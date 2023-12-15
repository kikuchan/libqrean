#include <stdio.h>

#include "qrmatrix.h"

int main() {
	qrmatrix_t qr = create_qrmatrix();

	qrmatrix_write_string(&qr, "Hello, world");

	qrmatrix_dump(&qr);
}
