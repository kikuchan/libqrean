#include "qrean.h"

extern qrean_code_t qrean_code_qr;
extern qrean_code_t qrean_code_mqr;
extern qrean_code_t qrean_code_rmqr;

extern qrean_code_t qrean_code_ean8;
extern qrean_code_t qrean_code_ean13;
extern qrean_code_t qrean_code_upca;
extern qrean_code_t qrean_code_code39;
extern qrean_code_t qrean_code_code93;
extern qrean_code_t qrean_code_itf;
extern qrean_code_t qrean_code_nw7;

static const qrean_code_t *codes[] = {
	&qrean_code_qr,
	&qrean_code_mqr,
	&qrean_code_rmqr,
	&qrean_code_ean8,
	&qrean_code_ean13,
	&qrean_code_upca,
	&qrean_code_code39,
	&qrean_code_code93,
	&qrean_code_itf,
	&qrean_code_nw7,
};
