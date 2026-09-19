#include "host_pacing.h"

static bool prepare(
	dttr_host_pacing *state, int cap, bool fixed_policy, bool steady_scene
) {
	if (!state) {
		return false;
	}
	if (state->cap != cap || state->fixed_policy != fixed_policy) {
		state->next_deadline_ns = 0;
	}
	state->cap = cap;
	state->fixed_policy = fixed_policy;
	if (!steady_scene || cap <= 0 || (!fixed_policy && cap >= 31)) {
		state->next_deadline_ns = 0;
		return false;
	}
	return true;
}

uint64_t dttr_host_pacing_wait_ns(
	dttr_host_pacing *state, uint64_t now, int cap, bool fixed_policy, bool steady_scene
) {
	if (!prepare(state, cap, fixed_policy, steady_scene)) {
		return 0;
	}
	return state->next_deadline_ns > now ? state->next_deadline_ns - now : 0;
}

void dttr_host_pacing_complete(
	dttr_host_pacing *state,
	uint64_t tick_start_ns,
	int cap,
	bool fixed_policy,
	bool steady_scene,
	bool advanced
) {
	if (!prepare(state, cap, fixed_policy, steady_scene) || (!fixed_policy && !advanced)) {
		return;
	}
	const uint64_t step_ns = 1000000000ull / (uint64_t)cap;
	state->next_deadline_ns = tick_start_ns > UINT64_MAX - step_ns
		? UINT64_MAX
		: tick_start_ns + step_ns;
}
