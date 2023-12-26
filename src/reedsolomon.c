#include "galois.h"
#include "utils.h"

gf2_poly_t *rs_init_generator_polynomial(gf2_poly_t *ans)
{
	int i;

	CREATE_GF2_POLY(a, GF2_POLY_DEGREE(ans));
	GF2_POLY_COEFF(a, 0) = 1;

	for (i = 0; i <= GF2_POLY_DEGREE(ans) - 1; i++) {
		// (x - a^i)
		CREATE_GF2_POLY(b, 1);
		GF2_POLY_COEFF(b, 0) = gf2_pow_a(i);
		GF2_POLY_COEFF(b, 1) = gf2_pow_a(0);

		gf2_poly_mul(a, a, b);
	}

	gf2_poly_copy(ans, a);
	return ans;
}

static void rs_solve_key_equation(gf2_poly_t *sigma, gf2_poly_t *omega, const gf2_poly_t *a, const gf2_poly_t *b)
{
	int maxdegree = MAX(gf2_poly_get_real_degree(a), gf2_poly_get_real_degree(b));
	CREATE_GF2_POLY(m, maxdegree);
	CREATE_GF2_POLY(n, maxdegree);

	// swap
	if (gf2_poly_get_real_degree(a) < gf2_poly_get_real_degree(b)) {
		gf2_poly_copy(m, b);
		gf2_poly_copy(n, a);
	} else {
		gf2_poly_copy(m, a);
		gf2_poly_copy(n, b);
	}

	CREATE_GF2_POLY(x, maxdegree);
	CREATE_GF2_POLY(y, maxdegree);
	GF2_POLY_COEFF(y, 0) = 1;

	while (!gf2_poly_is_zero(n) && gf2_poly_get_real_degree(n) >= gf2_poly_get_real_degree(y)) {
		CREATE_GF2_POLY(q, GF2_POLY_DEGREE(m));
		CREATE_GF2_POLY(r, GF2_POLY_DEGREE(m));
		CREATE_GF2_POLY(z, GF2_POLY_DEGREE(m));

		gf2_poly_divmod(q, r, m, n);

		gf2_poly_add(z, gf2_poly_mul(q, q, y), x); // z = q * y + x;

		gf2_poly_copy(x, y);
		gf2_poly_copy(y, z); // x = y; y = z;
		gf2_poly_copy(m, n);
		gf2_poly_copy(n, r); // m = n; n = r;
	}

	// divide both y and n by y[0];
	CREATE_GF2_POLY(h, 0);
	GF2_POLY_COEFF(h, 0) = GF2_POLY_COEFF(y, 0);

	if (sigma) gf2_poly_div(sigma, y, h);
	if (omega) gf2_poly_div(omega, n, h);
}

gf2_poly_t *rs_calc_parity(gf2_poly_t *ans, const gf2_poly_t *I, const gf2_poly_t *g)
{
	return gf2_poly_mod(ans, I, g);
}

int rs_fix_errors(gf2_poly_t *R, int error_words)
{
	// syndrome
	CREATE_GF2_POLY(S, error_words - 1);
	for (int i = 0; i < error_words; i++) {
		GF2_POLY_COEFF(S, i) = gf2_poly_calc(R, gf2_pow_a(i));
	}

	// No error
	if (gf2_poly_is_zero(S)) {
		return 0;
	}

	CREATE_GF2_POLY(z, error_words);
	GF2_POLY_COEFF(z, error_words) = 1; // z: x^7
	CREATE_GF2_POLY(sigma, error_words);
	CREATE_GF2_POLY(omega, error_words);
	rs_solve_key_equation(sigma, omega, z, S);

	CREATE_GF2_POLY(x, 1);
	GF2_POLY_COEFF(x, 1) = 1;
	CREATE_GF2_POLY(Denom, error_words + 1);
	CREATE_GF2_POLY(tmp, error_words);
	gf2_poly_mul(Denom, x, gf2_poly_dif(tmp, sigma));

	// Chien search & Forney algorithm
	int num_errors = 0;
	for (int pos = 0; pos <= GF2_POLY_DEGREE(R); pos++) {
		// Chien search: test whether Sigma(a^i) = 0
		if (gf2_poly_calc(sigma, gf2_pow_a(GF2_MAX_EXP - pos)) != 0) {
			continue;
		}

		// Forney algorithm:
		//   error_value = Omega(a^i) / (a^i * Sigma'(a^i));
		//               = Omega(a^i) / Denom(a^i)
		GF2_POLY_COEFF(R, pos) = gf2_add(GF2_POLY_COEFF(R, pos),
			gf2_div(gf2_poly_calc(omega, gf2_pow_a(GF2_MAX_EXP - pos)), gf2_poly_calc(Denom, gf2_pow_a(GF2_MAX_EXP - pos))));
		num_errors++;
	}

	if (num_errors != gf2_poly_get_real_degree(sigma)) {
		return -1;
	}

	return num_errors;
}
