#include "galois.h"

gf2_poly_t *rs_init_generator_polynomial(gf2_poly_t *ans);
gf2_poly_t *rs_calc_parity(gf2_poly_t *ans, const gf2_poly_t *I, const gf2_poly_t *g);
int rs_fix_errors(gf2_poly_t *r, int error_words);
