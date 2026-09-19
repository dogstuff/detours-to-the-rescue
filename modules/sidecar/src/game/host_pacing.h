#ifndef DTTR_HOST_PACING_H
#define DTTR_HOST_PACING_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	uint64_t next_deadline_ns;
	int cap;
	bool fixed_policy;
} dttr_host_pacing;

uint64_t dttr_host_pacing_wait_ns(
	dttr_host_pacing *state,
	uint64_t now,
	int cap,
	bool fixed_policy,
	bool steady_scene
);
void dttr_host_pacing_complete(
	dttr_host_pacing *state,
	uint64_t tick_start_ns,
	int cap,
	bool fixed_policy,
	bool steady_scene,
	bool advanced
);

#endif
