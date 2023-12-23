#ifndef __QR_DEBUG_H__
#define __QR_DEBUG_H__

#include <stdio.h>

#define QREAN_BANNER "[libqrean] "

int qrean_debug_printf(const char *fmt, ...);
void qrean_error(const char *message);

void qrean_on_debug_printf(int (*vprintf_like)(), void *opaque);
void qrean_on_error(void (*func)(const char *message));

const char *qrean_get_last_error();

#endif /* __QR_DEBUG_H__ */
