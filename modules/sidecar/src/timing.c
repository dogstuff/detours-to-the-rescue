#include "timing_private.h"

#include "mods/mods_private.h"

#include <dttr_log.h>

#include <windows.h>

#define DTTR_TIMING_DEFAULT_MAX_HOST_DELTA_NS 250000000ull

#define NS_PER_SEC 1000000000ull

typedef struct {
	DTTR_Mods_TimingPolicyRequest policy;
	bool policy_selected;
	bool fixed_timing;
	bool first_host_frame;
	bool sim_step_in_progress;
	bool render_reuses_previous_sim_state;
	bool render_frame_pending_advance;
	uint64_t last_time_ns;
	uint64_t host_frame_index;
	uint64_t render_frame_index;
	uint64_t simulation_tick_index;
	uint64_t host_delta_ns;
	uint64_t accumulator_ns;
	uint32_t sim_steps_due;
	uint32_t sim_steps_ran_this_host_frame;
	uint32_t sim_steps_deferred_this_host_frame;
	uint64_t sim_step_ns;
} DTTR_TimingState;

static DTTR_TimingState timing;

static uint64_t monotonic_time_ns() {
	LARGE_INTEGER counter = {0};
	LARGE_INTEGER frequency = {0};
	QueryPerformanceCounter(&counter);
	QueryPerformanceFrequency(&frequency);
	const uint64_t freq = (uint64_t)frequency.QuadPart;
	if (!freq) {
		return GetTickCount64() * 1000000ull;
	}

	const uint64_t ticks = (uint64_t)counter.QuadPart;
	const uint64_t seconds = ticks / freq;
	const uint64_t remainder = ticks % freq;
	if (seconds > UINT64_MAX / NS_PER_SEC) {
		return UINT64_MAX;
	}

	uint64_t ns = seconds * NS_PER_SEC;
	if (remainder <= UINT64_MAX / NS_PER_SEC) {
		ns += (remainder * NS_PER_SEC) / freq;
	} else {
		ns += (uint64_t)(((long double)remainder * (long double)NS_PER_SEC)
						 / (long double)freq);
	}

	return ns;
}

static bool ratio_to_step_ns(DTTR_Mods_RatioU32 hz, uint64_t *out_step_ns) {
	if (!out_step_ns || hz.num == 0 || hz.den == 0) {
		return false;
	}

	const uint64_t numerator = NS_PER_SEC * (uint64_t)hz.den;
	const uint64_t step_ns = numerator / hz.num;
	if (step_ns == 0) {
		return false;
	}

	*out_step_ns = step_ns;
	return true;
}

static void select_policy_once() {
	if (timing.policy_selected) {
		return;
	}

	timing.policy = (DTTR_Mods_TimingPolicyRequest){
		.struct_size = sizeof(DTTR_Mods_TimingPolicyRequest),
		.abi_version = DTTR_SDK_ABI_VERSION,
		.mode = DTTR_MODS_TIMING_NATIVE,
	};
	timing.policy_selected = true;

	if (!dttr_mods_select_timing_policy(&timing.policy)
		|| timing.policy.mode != DTTR_MODS_TIMING_FIXED_SIM_VARIABLE_RENDER) {
		timing.fixed_timing = false;
		return;
	}

	if (!ratio_to_step_ns(timing.policy.preferred_sim_hz, &timing.sim_step_ns)) {
		DTTR_LOG_ERROR(
			"Fixed DTTR timing requested without a valid preferred_sim_hz; using native "
			"timing"
		);
		timing.fixed_timing = false;
		return;
	}

	if (timing.policy.max_sim_steps_per_host_frame == 0) {
		timing.policy.max_sim_steps_per_host_frame = 1;
	}

	if (timing.policy.max_host_delta_ns == 0) {
		timing.policy.max_host_delta_ns = DTTR_TIMING_DEFAULT_MAX_HOST_DELTA_NS;
	}

	if (timing.policy.max_accumulator_debt_ns == 0) {
		timing.policy.max_accumulator_debt_ns = 2 * timing.sim_step_ns;
	}

	if (timing.policy.max_accumulator_debt_ns < timing.sim_step_ns) {
		DTTR_LOG_ERROR(
			"Fixed DTTR timing max_accumulator_debt_ns is smaller than one "
			"simulation step; using native timing"
		);
		timing.fixed_timing = false;
		return;
	}

	timing.fixed_timing = true;
	DTTR_LOG_INFO(
		"Selected DTTR fixed timing policy: %llu/%llu Hz, max_steps=%u",
		(unsigned long long)timing.policy.preferred_sim_hz.num,
		(unsigned long long)timing.policy.preferred_sim_hz.den,
		timing.policy.max_sim_steps_per_host_frame
	);
}

void dttr_timing_reset() {
	timing = (DTTR_TimingState){0};
	timing.first_host_frame = true;
}

bool dttr_timing_fixed_policy_active() {
	return timing.fixed_timing;
}

bool dttr_timing_render_reuses_previous_sim_state() {
	return timing.fixed_timing && timing.sim_steps_ran_this_host_frame == 0
		   && !timing.sim_step_in_progress;
}

void dttr_timing_host_frame_begin() {
	select_policy_once();

	const uint64_t now_ns = monotonic_time_ns();
	if (timing.first_host_frame) {
		timing.last_time_ns = now_ns;
		timing.first_host_frame = false;
	}

	timing.host_delta_ns = now_ns - timing.last_time_ns;
	timing.last_time_ns = now_ns;

	const uint64_t max_delta = timing.policy.max_host_delta_ns
								   ? timing.policy.max_host_delta_ns
								   : DTTR_TIMING_DEFAULT_MAX_HOST_DELTA_NS;
	if (timing.host_delta_ns > max_delta) {
		DTTR_LOG_WARN(
			"Clamping DTTR host delta from %llu ns to %llu ns",
			(unsigned long long)timing.host_delta_ns,
			(unsigned long long)max_delta
		);
		timing.host_delta_ns = max_delta;
	}

	timing.host_frame_index++;
	timing.sim_steps_ran_this_host_frame = 0;
	timing.sim_steps_deferred_this_host_frame = 0;

	if (timing.fixed_timing) {
		timing.accumulator_ns += timing.host_delta_ns;
		if (timing.accumulator_ns > timing.policy.max_accumulator_debt_ns) {
			timing.accumulator_ns = timing.policy.max_accumulator_debt_ns;
		}

		timing.sim_steps_due = (uint32_t)(timing.accumulator_ns / timing.sim_step_ns);
		if (timing.host_frame_index == 1 && timing.sim_steps_due == 0) {
			timing.sim_steps_due = 1;
		}

		if (timing.sim_steps_due > timing.policy.max_sim_steps_per_host_frame) {
			timing.sim_steps_due = timing.policy.max_sim_steps_per_host_frame;
		}
	} else {
		timing.sim_steps_due = 1;
	}

	DTTR_Mods_TimingFrameState frame_state;
	dttr_timing_frame_state(DTTR_MODS_TIMING_PHASE_HOST_FRAME_BEGIN, &frame_state);
	dttr_mods_timing_host_frame_begin(&frame_state);
}

bool dttr_timing_should_run_simulation_step() {
	if (timing.sim_steps_ran_this_host_frame >= timing.sim_steps_due) {
		return false;
	}

	DTTR_Mods_TimingFrameState frame_state;
	dttr_timing_frame_state(DTTR_MODS_TIMING_PHASE_BEFORE_SIMULATION_STEP, &frame_state);
	return dttr_mods_timing_should_run_simulation_step(&frame_state);
}

void dttr_timing_before_simulation_step() {
	timing.sim_step_in_progress = true;

	DTTR_Mods_TimingFrameState frame_state;
	dttr_timing_frame_state(DTTR_MODS_TIMING_PHASE_BEFORE_SIMULATION_STEP, &frame_state);
	dttr_mods_timing_before_simulation_step(&frame_state);
}

void dttr_timing_after_simulation_step() {
	timing.sim_step_in_progress = false;
	timing.sim_steps_ran_this_host_frame++;
	timing.simulation_tick_index++;
	if (timing.fixed_timing && timing.accumulator_ns >= timing.sim_step_ns) {
		timing.accumulator_ns -= timing.sim_step_ns;
	}

	DTTR_Mods_TimingFrameState frame_state;
	dttr_timing_frame_state(DTTR_MODS_TIMING_PHASE_AFTER_SIMULATION_STEP, &frame_state);
	dttr_mods_timing_after_simulation_step(&frame_state);
}

bool dttr_timing_has_deferred_simulation_step() {
	return timing.sim_steps_due > timing.sim_steps_ran_this_host_frame;
}

void dttr_timing_simulation_step_deferred() {
	if (!dttr_timing_has_deferred_simulation_step()) {
		return;
	}

	timing.sim_steps_deferred_this_host_frame = timing.sim_steps_due
												- timing.sim_steps_ran_this_host_frame;

	DTTR_Mods_TimingFrameState frame_state;
	dttr_timing_frame_state(DTTR_MODS_TIMING_PHASE_SIMULATION_STEP_DEFERRED, &frame_state);
	dttr_mods_timing_simulation_step_deferred(&frame_state);
}

void dttr_timing_before_render_frame(bool reuses_previous_sim_state, bool frame_open) {
	timing.render_reuses_previous_sim_state = reuses_previous_sim_state;

	DTTR_Mods_TimingFrameState frame_state;
	dttr_timing_frame_state(DTTR_MODS_TIMING_PHASE_BEFORE_RENDER_FRAME, &frame_state);

	if (frame_open) {
		frame_state.flags |= DTTR_MODS_TIMING_FRAME_FLAG_RENDER_FRAME_OPEN;
	}

	dttr_mods_timing_before_render_frame(&frame_state);
}

void dttr_timing_after_render_frame(bool reuses_previous_sim_state) {
	timing.render_reuses_previous_sim_state = reuses_previous_sim_state;

	DTTR_Mods_TimingFrameState frame_state;
	dttr_timing_frame_state(DTTR_MODS_TIMING_PHASE_AFTER_RENDER_FRAME, &frame_state);
	dttr_mods_timing_after_render_frame(&frame_state);

	timing.render_frame_pending_advance = true;
}

void dttr_timing_before_present_frame() {
	DTTR_Mods_TimingFrameState frame_state;
	dttr_timing_frame_state(DTTR_MODS_TIMING_PHASE_BEFORE_PRESENT_FRAME, &frame_state);
	dttr_mods_timing_before_present_frame(&frame_state);
}

void dttr_timing_after_present_frame() {
	DTTR_Mods_TimingFrameState frame_state;
	dttr_timing_frame_state(DTTR_MODS_TIMING_PHASE_AFTER_PRESENT_FRAME, &frame_state);
	dttr_mods_timing_after_present_frame(&frame_state);
}

void dttr_timing_host_frame_end() {
	DTTR_Mods_TimingFrameState frame_state;
	dttr_timing_frame_state(DTTR_MODS_TIMING_PHASE_HOST_FRAME_END, &frame_state);
	dttr_mods_timing_host_frame_end(&frame_state);

	if (timing.render_frame_pending_advance) {
		timing.render_frame_index++;
		timing.render_frame_pending_advance = false;
	}
}

void dttr_timing_frame_state(DTTR_Mods_TimingPhase phase, DTTR_Mods_TimingFrameState *out) {
	const uint64_t sim_step_ns = timing.fixed_timing ? timing.sim_step_ns : 0;
	float alpha = 0.0f;
	if (sim_step_ns && timing.accumulator_ns < sim_step_ns) {
		alpha = (float)((double)timing.accumulator_ns / (double)sim_step_ns);
	}

	*out = (DTTR_Mods_TimingFrameState){
		.struct_size = sizeof(DTTR_Mods_TimingFrameState),
		.abi_version = DTTR_SDK_ABI_VERSION,
		.phase = phase,
		.host_frame_index = timing.host_frame_index,
		.render_frame_index = timing.render_frame_index,
		.simulation_tick_index = timing.simulation_tick_index,
		.monotonic_time_ns = timing.last_time_ns,
		.host_delta_ns = timing.host_delta_ns,
		.sim_step_ns = sim_step_ns,
		.accumulator_ns = timing.accumulator_ns,
		.interpolation_alpha = alpha,
		.sim_steps_due = timing.sim_steps_due,
		.sim_steps_ran_this_host_frame = timing.sim_steps_ran_this_host_frame,
		.sim_steps_deferred_this_host_frame = timing.sim_steps_deferred_this_host_frame,
		.render_reuses_previous_sim_state
		= (phase == DTTR_MODS_TIMING_PHASE_BEFORE_RENDER_FRAME
		   || phase == DTTR_MODS_TIMING_PHASE_AFTER_RENDER_FRAME
		   || phase == DTTR_MODS_TIMING_PHASE_BEFORE_PRESENT_FRAME
		   || phase == DTTR_MODS_TIMING_PHASE_AFTER_PRESENT_FRAME)
		  && timing.render_reuses_previous_sim_state,
	};
}
