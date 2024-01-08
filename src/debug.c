#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"

#ifndef NO_DEBUG
static int (*qrean_debug_vprintf_cb)(void *opaque, const char *fmt, va_list ap);
static void *qrean_debug_vprintf_opaque = NULL;

#endif
static void (*qrean_error_cb)(const char *message);

const char *qrean_last_error = NULL;

#ifndef NO_PRINTF
int safe_vfprintf(FILE *fp, const char *fmt, va_list ap)
{
	int retval = 0;

	unsigned char buf[4096];
	retval = vsnprintf((char *)buf, sizeof(buf), fmt, ap);

	for (size_t i = 0; i < strlen((char *)buf); i++) {
		char hex[32];
		snprintf(hex, sizeof(hex), "[%02x]", buf[i]);
		if (buf[i] == '\r' || buf[i] == '\n' || (0x20 <= buf[i] && buf[i] < 0x7f))
			fputc(buf[i], fp);
		else
			fputs(hex, fp);
	}

	return retval;
}

int safe_fprintf(FILE *fp, const char *fmt, ...)
{
	va_list ap;
	int retval;
	va_start(ap, fmt);
	retval = safe_vfprintf(fp, fmt, ap);
	va_end(ap);
	return retval;
}
#endif

int qrean_debug_printf(const char *fmt, ...)
{
	va_list ap;
	int retval = 0;

#ifndef NO_DEBUG
	if (qrean_debug_vprintf_cb) {
		va_start(ap, fmt);
		retval = qrean_debug_vprintf_cb(qrean_debug_vprintf_opaque, fmt, ap);
		va_end(ap);
	}
#endif

	return retval;
}

void qrean_on_debug_vprintf(int (*vfprintf_like)(void *opaque, const char *fmt, va_list ap), void *opaque)
{
#ifndef NO_DEBUG
	qrean_debug_vprintf_cb = vfprintf_like;
	qrean_debug_vprintf_opaque = opaque;
#endif
}

void qrean_error(const char *message)
{
	if (qrean_error_cb) {
		if (qrean_error_cb) qrean_error_cb(message);
	} else {
		qrean_debug_printf(QREAN_BANNER "ERROR: %s\n", message);
	}

	qrean_last_error = message;
}

void qrean_on_error(void (*func)(const char *message))
{
#ifndef NO_DEBUG
	qrean_error_cb = func;
#endif
}

/* returns NULL if no error */
const char *qrean_get_last_error()
{
	return qrean_last_error;
}
