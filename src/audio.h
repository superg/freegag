#pragma once

#include "runtime_types.h"

namespace freegag
{

uint32_t create_runtime_sound_handle(const RuntimePcmFormat *source_format);

void destroy_runtime_sound_handle(uint32_t handle);

uint32_t queue_runtime_sound_data(uint32_t handle, const void *data, uint32_t size, int32_t replace);

uint32_t pause_runtime_sound(uint32_t handle, int32_t reset_timing);

uint32_t resume_runtime_sound(uint32_t handle, int32_t reset_timing);

void set_runtime_sound_loop_value(uint32_t handle, uint32_t value);

uint32_t query_runtime_sound_status(uint32_t handle, RuntimeSoundStatus *status);

uint32_t set_runtime_sound_playback_marker(uint32_t handle, uint32_t marker);

uint32_t set_runtime_sound_schedule_marker(uint32_t handle, uint32_t marker);

uint32_t restart_runtime_sound_data(uint32_t handle);

uint32_t fade_out_runtime_sound(uint32_t handle, int32_t duration_ms, int32_t reset_timing);

uint32_t fade_in_runtime_sound(uint32_t handle, int32_t duration_ms, int32_t reset_timing);

uint32_t set_runtime_sound_volume(uint32_t handle, uint8_t volume);

uint32_t shutdown_runtime_sound();

void toggle_runtime_sound_state();

uint32_t pause_runtime_sound_output(int32_t close_output);

uint32_t resume_runtime_sound_output();

void initialize_runtime_sound();

} // namespace freegag
