#include "stdint.h"

unsigned long strlen(const char* s) {
	unsigned long n = 0;
	while (*s++) {
		n++;
  }
	return n;
}

unsigned long strspn(const char* s1, const char* s2) {
  const char* p;
  for (p = s1; *s1; s1++) {
    for (const char* t = s2; *t != *s1; t++) {
      if (*t == '\0') {
        return s1 - p;
      }
    }
  }
  return s1 - p;
}

unsigned long strcspn(const char* s1, const char* s2) {
  const char* p;
  for (p = s1; *s1; s1++) {
    for (const char* t = s2; *t; t++) {
      if (*t == *s1) {
        return s1 - p;
      }
    }
  }
  return s1 - p;
}

char* strchr(const char* s, int c) {
	while (*s) {
		if (*s == c) {
			return (char*)s;
    }
		s++;
	}
	return NULL;
}

static inline unsigned char tolowercase(unsigned char c) {
  if (c >= 'A' && c <= 'Z') {
    c -= 'A' - 'a';
  } 
  return c;
}

int strcasecmp(const char* s1, const char* s2) {
  int c1, c2;
  do {
    c1 = tolowercase(*s1++);
    c2 = tolowercase(*s2++);
  } while (c1 == c2 && c1 != 0); 
  return c1 - c2;
}

void* memchr(const void* s, int c, unsigned long n) {
	unsigned char* us = (unsigned char*)s;
	for (unsigned long i = 0; i < n;) {
		if (us[i] == (unsigned char)c) {
			return us + i;
    }
		i++;
	}
	return NULL;
}
