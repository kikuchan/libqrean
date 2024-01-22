#include "stdint.h"
#include "tinymm.h"

void* malloc(unsigned long len) {
  return tinymm_malloc(len);
}

void free(void* p) {
  tinymm_free(p);
}

void* memset(void* p, int n, unsigned long len) {
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
  void *ptr = malloc(len * size);
  if (ptr) memset(ptr, 0, len * size);
  return ptr;
}
