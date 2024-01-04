#pragma once

void* memset(void* p, int len, unsigned long n);
void* memcpy(void* p1, const void* p2, unsigned long len);
void* malloc(unsigned long len);
void free(void* p);
void* calloc(unsigned long len, unsigned long size);
