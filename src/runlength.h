#ifndef __QR_RUNLENGTH_H__
#define __QR_RUNLENGTH_H__

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#define MAX_RUNLENGTH (10)

#define RUNLENGTH_SIZE(rl) (MAX_RUNLENGTH)

typedef unsigned int runlength_count_t;
typedef unsigned int runlength_size_t;

typedef struct {
	runlength_count_t ringbuf[MAX_RUNLENGTH];
	size_t idx;
	uint32_t last_value;
} runlength_t;

runlength_t create_runlength();
void runlength_init(runlength_t *rl);

runlength_count_t runlength_get_count(runlength_t *rl, runlength_size_t back);
runlength_count_t runlength_latest_count(runlength_t *rl);
runlength_count_t runlength_sum(runlength_t *rl, runlength_size_t s, runlength_size_t e);

void runlength_next(runlength_t *rl);

// returns 1 if value changed
int runlength_push_value(runlength_t *rl, uint32_t value);

void runlength_count(runlength_t *rl);
void runlength_count_add(runlength_t *rl, runlength_count_t n);
void runlength_next_and_count(runlength_t *rl);
void runlength_next_and_count_add(runlength_t *rl, runlength_count_t n);

int runlength_match_exact(runlength_t *rl, runlength_size_t N, ...);
int runlength_match_ratio(runlength_t *rl, runlength_size_t N, ...);

void runlength_dump(runlength_t *rl, runlength_size_t N);

#endif /* __QR_RUNLENGTH_H__ */
