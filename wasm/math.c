#include "math.h"

#ifndef USE_JS_MATH

float roundf(float f) {
  return (float)round(f);
}

double atan(double x) { // Approximation
  if (x > 1.0) {
    return 1.5708 - atan(1.0 / x);
  }
  return (12.0 * x * x + 45.0) * x / (27.0 * x * x + 45.0);
}

double atan2(double y, double x) {
  if (x == 0.0 && y == 0.0) {
    return 0.0;
  }
  if (x > 0.0) {
    return atan(y / x);
  }
  if (x < 0.0 && y >= 0.0) {
    return atan(y / x) + M_PI;
  }
  if (x < 0.0 && y < 0.0) {
    return atan(y / x) - M_PI;
  }
  if (x == 0.0 && y > 0.0) {
    return M_PI / 2.0;
  }
  return -M_PI / 2.0;
}

double __ieee754_pow(double n, double m);

double pow(double n, double m) {
	return __ieee754_pow(n, m);
}

#endif

float atan2f(float y, float x) {
  return (float)atan2(y, x);
}

int isnan(float f) {
  return f == NAN;
}
