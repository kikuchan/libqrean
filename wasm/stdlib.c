#include "stdint.h"
#include "tinymm.h"

#define MEM_SIZE (10 * 1024 * 1024)

#define MEM_TOP (256 * 1024)
#define N_ENTRIES 1000

void memreset() {
  tinymm_init((void*)MEM_TOP, MEM_SIZE - MEM_TOP, N_ENTRIES);
}

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
  return malloc(len * size);
}
