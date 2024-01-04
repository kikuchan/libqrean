#include "stdint.h"

//#define BUF_SIZE (1 * 1024 * 1024) // 1MB
#define BUF_SIZE (1 * 1024) // 1kB - ok
//#define BUF_SIZE (10 * 1024) // 10kB - ok
//#define BUF_SIZE (50 * 1024) // 50kB - ok
//#define BUF_SIZE (50 * 1024) // 100kB - ng
char buf[BUF_SIZE];
int p = 0;
void memreset() {
  p = 0;
}

void* malloc(unsigned long len) {
  if (p + len >= BUF_SIZE) {
    return NULL;
  }
  void* res = (void*)(buf + p);
  p += len;
  return res;
}

void free(void* p) {
  // don't free memory
}

void* memset(void* p, int len, unsigned long n) {
  for (int i = 0; i < len; i++) {
    ((char*)p)[i] = (char)n;
  }
  return p;
}
void* memcpy(void* p1, const void* p2, unsigned long len) {
  for (int i = 0; i < len; i++) {
    ((char*)p1)[i] = ((const char*)p2)[i];
  }
  return p1;
}

void* calloc(unsigned long len, unsigned long size) {
  return malloc(len * size);
}
