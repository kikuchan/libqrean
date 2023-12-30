#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>

#include "bitstream.h"
#include "runlength.h"

runlength_t create_runlength()
{
	runlength_t rl = {};
	runlength_init(&rl);
	return rl;
}

void runlength_init(runlength_t *rl)
{
	// 0 never matches
	runlength_next(rl);
	runlength_next(rl);
}

runlength_count_t runlength_get_count(runlength_t *rl, runlength_size_t back)
{
	assert(back < MAX_RUNLENGTH);
	return rl->ringbuf[(rl->idx - back + MAX_RUNLENGTH) % MAX_RUNLENGTH];
}

runlength_count_t runlength_latest_count(runlength_t *rl)
{
	return runlength_get_count(rl, 0);
}

runlength_count_t runlength_sum(runlength_t *rl, runlength_size_t s, runlength_size_t n)
{
	runlength_count_t sum = 0;
	for (runlength_size_t i = 0; i < n; i++) {
		sum += runlength_get_count(rl, s + i);
	}
	return sum;
}

void runlength_next(runlength_t *rl)
{
	rl->idx = (rl->idx + 1) % MAX_RUNLENGTH;
	rl->ringbuf[rl->idx] = 0;
}

int runlength_push_value(runlength_t *rl, uint32_t value)
{
	if (rl->last_value != value) {
		rl->last_value = value;
		runlength_next_and_count(rl);
		return 1;
	}
	runlength_count(rl);
	return 0;
}

void runlength_count_add(runlength_t *rl, runlength_count_t n)
{
	rl->ringbuf[rl->idx] += n;
}

void runlength_count(runlength_t *rl)
{
	runlength_count_add(rl, 1);
}

void runlength_next_and_count(runlength_t *rl)
{
	runlength_next(rl);
	runlength_count(rl);
}

void runlength_next_and_count_add(runlength_t *rl, runlength_count_t n)
{
	runlength_next(rl);
	runlength_count_add(rl, n);
}

// eg; 1 2 0 2 1 matches 1 2 X 2 1
int runlength_match_exact(runlength_t *rl, float div, runlength_size_t N, ...)
{
	va_list ap;

	assert(0 < N && N <= MAX_RUNLENGTH);

	va_start(ap, N);

	for (runlength_size_t i = 0; i < N; i++) {
		runlength_count_t arg = va_arg(ap, runlength_size_t);
		if (arg && round(runlength_get_count(rl, N - i - 1) / div) != arg) {
			va_end(ap);
			return 0;
		}
	}

	va_end(ap);
	return 1;
}

// eg; 2 0 1 3 1 matches 4 X 2 6 2
int runlength_match_ratio(runlength_t *rl, runlength_size_t N, ...)
{
	va_list ap;

	assert(0 < N && N <= MAX_RUNLENGTH);

	va_start(ap, N);

	runlength_count_t total_count = 0, total_ratio = 0;
	runlength_count_t ratio[N];
	for (runlength_size_t i = 0; i < N; i++) {
		ratio[i] = va_arg(ap, int);
		if (!ratio[i]) continue;

		total_ratio += ratio[i];
		total_count += runlength_get_count(rl, N - i - 1);
	}

	va_end(ap);

	if (!total_ratio || !total_count) return 0;

	for (runlength_size_t i = 0; i < N; i++) {
		const int scale = 16;
		runlength_count_t count_i = runlength_get_count(rl, N - i - 1) * scale;
		if (!count_i) return 0; // never matches, even if ratio[i] is zero

		if (!ratio[i]) continue; // skip (wildcard)

		int avg = total_count * scale / total_ratio;
		int err = avg;

		if (!(ratio[i] * avg - err <= count_i && count_i <= ratio[i] * avg + err)) return 0;
	}

	return 1;
}

void runlength_dump(runlength_t *rl, FILE *out)
{
	runlength_size_t N = MAX_RUNLENGTH;
	for (runlength_size_t i = 0; i < N; i++) {
		fprintf(out, "%2d ", runlength_get_count(rl, N - i - 1));
	}
	fprintf(out, "\n");
}
