#pragma once

#include "runtime_types.h"

namespace gag
{

uint32_t create_runtime_sound_handle(const RuntimePcmFormat *source_format);

void destroy_runtime_sound_handle(uint32_t handle);

uint32_t queue_runtime_sound_data(uint32_t handle, void *data, uint32_t size, int32_t replace);

uint32_t start_runtime_sound(uint32_t handle, int32_t reset_timing);

uint32_t stop_runtime_sound(uint32_t handle, int32_t reset_timing);

void set_runtime_sound_loop_value(uint32_t handle, uint32_t value);

RuntimeSoundSlot *get_runtime_sound_slot(uint32_t handle);

uint32_t fade_out_runtime_sound(uint32_t handle, int32_t duration_ms, int32_t reset_timing);

uint32_t fade_in_runtime_sound(uint32_t handle, int32_t duration_ms, int32_t reset_timing);

uint32_t set_runtime_sound_volume(uint32_t handle, uint8_t volume);

uint32_t shutdown_runtime_sound();

void toggle_runtime_sound_state();

uint32_t pause_runtime_sound_output(int32_t close_output);

uint32_t resume_runtime_sound_output();

uint32_t ensure_runtime_sound_ready(const RuntimePcmFormat *format, uint32_t mixer_argument);

void initialize_runtime_sound();

uint32_t runtime_pcm_formats_equal(const RuntimePcmFormat *left, const RuntimePcmFormat *right);

uint32_t calculate_runtime_pcm_conversion(const RuntimePcmFormat *source, const RuntimePcmFormat *destination, uint16_t *conversion_flags);

void mix_runtime_sound_8bit_mono(uint32_t marker);

void mix_runtime_sound_8bit_stereo(uint32_t marker);

void mix_runtime_sound_16bit_mono(uint32_t marker);

void mix_runtime_sound_16bit_stereo(uint32_t marker);

} // namespace gag
