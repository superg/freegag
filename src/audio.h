#pragma once

#include "runtime_types.h"

namespace gag
{

uint32_t create_runtime_sound_handle(WAVEFORMATEX *source_format);

void destroy_runtime_sound_handle(uint32_t handle);

uint32_t queue_runtime_sound_data(uint32_t handle, void *data, uint32_t size, int32_t replace);

uint32_t start_runtime_sound(uint32_t handle, int32_t reset_timing);

uint32_t stop_runtime_sound(uint32_t handle, int32_t reset_timing);

void set_runtime_sound_loop_value(uint32_t handle, uint32_t value);

RuntimeSoundSlot *get_runtime_sound_slot(uint32_t handle);

uint32_t fade_out_runtime_sound(uint32_t handle, int32_t duration_ms, int32_t reset_timing);

uint32_t fade_in_runtime_sound(uint32_t handle, int32_t duration_ms, int32_t reset_timing);

uint32_t set_runtime_sound_volume(uint32_t handle, uint8_t volume);

void CALLBACK runtime_wave_out_callback(HWAVEOUT wave_out, UINT message, DWORD_PTR instance, DWORD_PTR parameter_1, DWORD_PTR parameter_2);

uint32_t shutdown_runtime_sound();

void toggle_runtime_sound_state();

uint32_t pause_runtime_sound_output(int32_t close_output);

uint32_t resume_runtime_sound_output();

uint32_t ensure_runtime_sound_ready(WAVEFORMATEX *format, uint32_t mixer_argument);

DWORD WINAPI run_runtime_sound_thread(LPVOID parameter);

LRESULT CALLBACK runtime_sound_window_procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

void initialize_runtime_sound_class(HINSTANCE instance);

uint32_t runtime_wave_formats_equal(const WAVEFORMATEX *left, const WAVEFORMATEX *right);

uint32_t calculate_runtime_wave_conversion(const WAVEFORMATEX *source, const WAVEFORMATEX *destination, uint16_t *conversion_flags);

void cleanup_runtime_sound_format_buffer();

void mix_runtime_sound_8bit_mono(uint32_t marker);

void mix_runtime_sound_8bit_stereo(uint32_t marker);

void mix_runtime_sound_16bit_mono(uint32_t marker);

void mix_runtime_sound_16bit_stereo(uint32_t marker);

uint32_t initialize_runtime_wave_out_mixer(WAVEFORMATEX *format, uint32_t unused_argument);

} // namespace gag
