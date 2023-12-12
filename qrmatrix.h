#include <stdint.h>

#include "types.h"
#include "bitstream.h"
#include "qrstream.h"

#define MAX_QR_VERSION (40)
#define SYMBOL_SIZE_FOR(version) (17 + (version) * 4)

typedef struct _qrmatrix_t qrmatrix_t;
typedef struct { uint8_t x; uint8_t y; uint8_t v; } qrpos_t;

typedef qrpos_t (*qrmatrix_iterator_t)(qrmatrix_t *qr, bitpos_t i, void *opaque);

struct _qrmatrix_t {
	qr_version_t version;
	qr_errorlevel_t level;
	qr_maskpattern_t mask;

	uint8_t symbol_size;

#if defined(USE_MALLOC_BUFFER) && defined(NO_MALLOC)
#error "Specify both of USE_MALLOC_BUFFER and NO_MALLOC doesn't make sense"
#endif
#ifdef USE_MALLOC_BUFFER
	uint8_t *buffer;
#else
	uint8_t buffer[(SYMBOL_SIZE_FOR(MAX_QR_VERSION)*SYMBOL_SIZE_FOR(MAX_QR_VERSION) + 7) / 8];
#endif

	qrmatrix_iterator_t iter;
	void *opaque;

#ifndef NO_CALLBACK
	bit_t (*set_pixel)(qrmatrix_t *qr, bitpos_t x, bitpos_t y, bitpos_t pos, bit_t v);
	bit_t (*get_pixel)(qrmatrix_t *qr, bitpos_t x, bitpos_t y, bitpos_t pos);
#endif
};

bit_t qrmatrix_init(qrmatrix_t *qr, qr_version_t version, qr_errorlevel_t level, qr_maskpattern_t mask);
void qrmatrix_deinit(qrmatrix_t *qr);

#ifndef USE_MALLOC_ONLY
qrmatrix_t create_qrmatrix(qr_version_t version, qr_errorlevel_t level, qr_maskpattern_t mask);
qrmatrix_t create_qrmatrix_for_string(qr_version_t version, qr_errorlevel_t level, const char *src);
#endif

qrmatrix_t *new_qrmatrix(qr_version_t version, qr_errorlevel_t level, qr_maskpattern_t mask);
qrmatrix_t *new_qrmatrix_for_string(qr_version_t version, qr_errorlevel_t level, const char *src);

void qrmatrix_free(qrmatrix_t *qr);


void qrmatrix_set_pixel(qrmatrix_t *qr, int_fast8_t x, int_fast8_t y, bit_t v);
bit_t qrmatrix_get_pixel(qrmatrix_t *qr, int_fast8_t x, int_fast8_t y);

int qrmatrix_fix_errors(qrmatrix_t *qr);

void qrmatrix_dump(qrmatrix_t *qr, int padding);


bitstream_t qrmatrix_create_bitstream(qrmatrix_t *qr, bitstream_iterator_t iter);
bitstream_t qrmatrix_create_bitstream_for_composed_data(qrmatrix_t *qr);

void qrmatrix_write_format_info(qrmatrix_t *qr);
void qrmatrix_write_version_info(qrmatrix_t *qr);
void qrmatrix_write_timing_pattern(qrmatrix_t *qr);
void qrmatrix_write_finder_pattern(qrmatrix_t *qr);
void qrmatrix_write_alignment_pattern(qrmatrix_t *qr);

void qrmatrix_write_data(qrmatrix_t *qr, qrstream_t *qrs);
void qrmatrix_read_data(qrmatrix_t *qr, qrstream_t *qrs);

void qrmatrix_write_function_patterns(qrmatrix_t *qr);
void qrmatrix_write_all(qrmatrix_t *qr, qrstream_t *qrs);

void qrmatrix_write_string(qrmatrix_t *qr, const char *src);

#ifndef NO_CALLBACK
void qrmatrix_on_set_pixel(qrmatrix_t *qr, bit_t (*set_pixel)(qrmatrix_t *qr, bitpos_t x, bitpos_t y, bitpos_t pos, bit_t v));
void qrmatrix_on_get_pixel(qrmatrix_t *qr, bit_t (*get_pixel)(qrmatrix_t *qr, bitpos_t x, bitpos_t y, bitpos_t pos));
#endif

