#ifndef __QR_GALOIS_H__
#define __QR_GALOIS_H__

#include <stdint.h>
#include <string.h>

#include "bitstream.h"

typedef uint8_t gf2_value_t;

#define GF2_MAX_EXP (255)

#define GF2_POLY_SIZE(deg)   (2 + (deg))
#define GF2_POLY_DEGREE(x)   ((x)[0])
#define GF2_POLY_COEFF(x, i) ((x)[1 + (i)])
// #define GF2_POLY_COEFF(x, i) ((x)[1 + GF2_POLY_DEGREE(x) - (i)])
#define CREATE_GF2_POLY(name, degree)          \
	gf2_value_t name[GF2_POLY_SIZE((degree))]; \
	memset(name, 0, sizeof(name));             \
	GF2_POLY_DEGREE(name) = (degree);

typedef gf2_value_t gf2_poly_t;

gf2_value_t gf2_add(gf2_value_t a, gf2_value_t b);
gf2_value_t gf2_mul(gf2_value_t a, gf2_value_t b);
gf2_value_t gf2_pow(gf2_value_t a, int exp);
gf2_value_t gf2_div(gf2_value_t a, gf2_value_t b);
gf2_value_t gf2_pow_a(int exp);
gf2_value_t gf2_log_a(gf2_value_t val);

gf2_poly_t *gf2_poly_mul(gf2_poly_t *ans, const gf2_poly_t *a, const gf2_poly_t *b);
gf2_poly_t *gf2_poly_div(gf2_poly_t *ans, const gf2_poly_t *a, const gf2_poly_t *b);
void gf2_poly_divmod(gf2_poly_t *q, gf2_poly_t *r, const gf2_poly_t *a, const gf2_poly_t *b);

gf2_poly_t *gf2_poly_mod(gf2_poly_t *ans, const gf2_poly_t *a, const gf2_poly_t *b);
gf2_poly_t *gf2_poly_add(gf2_poly_t *ans, const gf2_poly_t *a, const gf2_poly_t *b);
gf2_poly_t *gf2_poly_dif(gf2_poly_t *ans, const gf2_poly_t *a);
gf2_value_t gf2_poly_calc(const gf2_poly_t *a, gf2_value_t value);

void gf2_poly_copy(gf2_poly_t *dst, const gf2_poly_t *src);

int gf2_poly_get_real_degree(const gf2_poly_t *a);
int gf2_poly_is_zero(const gf2_poly_t *poly);
void gf2_solve_key_equation(gf2_poly_t *sigma, gf2_poly_t *omega, const gf2_poly_t *a, const gf2_poly_t *b);

void gf2_poly_dump(const gf2_poly_t *poly, FILE *out);

#endif /* __QR_GALOIS_H__ */
