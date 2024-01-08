#include <stddef.h>

#define FILE uint32_t

#define fprintf(...) (void)0
#define vfprintf(...) (void)0
#define fputc(...) (void)0

#define stdout (FILE*)0
#define stderr (FILE*)0
