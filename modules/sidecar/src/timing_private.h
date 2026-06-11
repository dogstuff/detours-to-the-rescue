#ifndef DTTR_TIMING_H
#define DTTR_TIMING_H

#include <stdbool.h>

#include <dttr_mods.h>

void dttr_timing_reset();
void dttr_timing_host_frame_begin();
bool dttr_timing_should_run_simulation_step();
bool dttr_timing_has_deferred_simulation_step();
void dttr_timing_before_simulation_step();
void dttr_timing_after_simulation_step();
void dttr_timing_simulation_step_deferred();
void dttr_timing_before_render_frame(bool reuses_previous_sim_state);
void dttr_timing_after_render_frame(bool reuses_previous_sim_state);
void dttr_timing_before_present_frame();
void dttr_timing_after_present_frame();
void dttr_timing_host_frame_end();
void dttr_timing_frame_state(DTTR_Mods_TimingPhase phase, DTTR_Mods_TimingFrameState *out);

#endif // DTTR_TIMING_H
