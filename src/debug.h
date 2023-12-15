#ifndef __QR_DEBUG_H__
#define __QR_DEBUG_H__

#include <stdio.h>

#ifdef DEBUG
#define BANNER            "[libqrean] "
#define debug_printf(...) fprintf(stderr, __VA_ARGS__)
#else
#define debug_printf(...)
#endif

#define fatal(s)                 \
	do {                         \
		debug_printf("%s\n", s); \
		assert(0);               \
	} while (0)

#endif /* __QR_DEBUG_H__ */
