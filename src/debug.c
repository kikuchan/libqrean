#include <stdarg.h>
#include <stdio.h>

#include "debug.h"

#ifndef NO_DEBUG
static int (*qrean_debug_vprintf_cb)(const char *fmt, va_list ap);
static void (*qrean_error_cb)(const char *message);
#endif

int qrean_debug_printf(const char *fmt, ...) {
#ifndef NO_DEBUG
	va_list ap;
	int retval = 0;

	if (qrean_debug_vprintf_cb) {
		va_start(ap, fmt);
		if (qrean_debug_vprintf_cb) retval = qrean_debug_vprintf_cb(fmt, ap);
		va_end(ap);
	}

	return retval;
#endif
}

void qrean_on_debug_printf(int (*func)(const char *fmt, va_list ap)) {
#ifndef NO_DEBUG
	qrean_debug_vprintf_cb = func;
#endif
}

const char *qrean_last_error = NULL;

void qrean_error(const char *fmt, ...) {
#ifndef NO_DEBUG
	static char buf[1024];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	if (qrean_error_cb) {
		if (qrean_error_cb) qrean_error_cb(buf);
	} else {
		qrean_debug_printf(QREAN_BANNER "ERROR: %s\n", buf);
	}

	qrean_last_error = buf;
#endif
}

void qrean_on_error(void (*func)(const char *message)) {
#ifndef NO_DEBUG
	qrean_error_cb = func;
#endif
}

/* returns NULL if no error */
const char *qrean_get_last_error() {
	return qrean_last_error;
}
