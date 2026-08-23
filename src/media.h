#pragma once

#include "runtime_types.h"

namespace gag
{

RuntimeMediaBackend *create_runtime_bitmap_backend(uint32_t unused, uint32_t extension_bytes, void *bitmap_data);

uint32_t configure_runtime_bitmap_backend(void *identity, const DisplaySceneDescriptor *descriptor, void *callback, uint32_t flags);

uint32_t configure_runtime_animation_backend(void *identity, const DisplaySceneDescriptor *descriptor, const void *comparison_palette, uint32_t flags, RuntimeAnimationCallback callback);

void configure_runtime_resource_palette(RuntimeResourceObject *resource);

void build_runtime_palette_index_remap(RuntimeMediaBackend *backend);

uint8_t convert_runtime_bitmap_to_surface(RuntimeMediaBackend *backend);

void finalize_runtime_media_backend(void *identity);

void fail_runtime_animation(RuntimeMediaBackend *backend, uint32_t error);

void pause_all_runtime_animations();

void resume_all_runtime_animations();

RuntimeAnimationControlResult process_runtime_animation_control(RuntimeAnimationBackend *backend, uint32_t current_time, uint32_t *wait_milliseconds);

void schedule_runtime_animation_frame(RuntimeMediaBackend *backend, uint32_t current_time);

bool acquire_runtime_animation_frame(RuntimeAnimationBackend *backend);

void decode_runtime_animation_frame_chunks(RuntimeAnimationBackend *backend);

void complete_runtime_animation_frame(RuntimeAnimationBackend *backend);

void process_runtime_animation_audio_chunks(RuntimeAnimationBackend *backend);

DWORD WINAPI run_runtime_animation_thread(void *backend);

void decode_runtime_animation_palette(RuntimeMediaBackend *backend);

void decode_runtime_animation_mvz8(RuntimeMediaBackend *backend);

void decode_runtime_animation_mvz5(RuntimeMediaBackend *backend);

void decode_runtime_animation_literal(RuntimeMediaBackend *backend);

void decode_runtime_animation_byte_run(RuntimeMediaBackend *backend);

void decode_runtime_animation_delta_flc(RuntimeMediaBackend *backend);

void ignore_runtime_animation_chunk_11();

void ignore_runtime_animation_chunk_12();

void ignore_runtime_animation_chunk_13();

RuntimeAnimationBackend *create_runtime_animation_backend(uint32_t unused, void *data, uint32_t extension_bytes, uint32_t storage);

RuntimeMediaBackend *acquire_runtime_media_backend(void *identity);

uint32_t get_runtime_media_backend_type(void *identity);

uint8_t classify_runtime_media_data(const void *data);

uint32_t read_compressor_input(void *destination, uint32_t requested_size);

void set_runtime_media_backend_scale(void *identity, uint32_t scale_x, uint32_t scale_y);

uint32_t stop_runtime_animation_backend(void *identity);

void *get_locked_runtime_media_extension(void *identity);

uint32_t destroy_runtime_media_backend(void *identity);

} // namespace gag
