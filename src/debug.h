#ifndef __QR_DEBUG_H__
#define __QR_DEBUG_H__

#include <stdio.h>

#define QREAN_BANNER "[libqrean] "

int qrean_debug_printf(const char *fmt, ...);
void qrean_error(const char *fmt, ...);

void qrean_on_debug_printf(int (*func)(const char *fmt, va_list ap));
void qrean_on_error(void (*func)(const char *message));

const char *qrean_get_last_error();

#endif /* __QR_DEBUG_H__ */
