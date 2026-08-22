#pragma once

#include "runtime_types.h"

namespace gag
{

void render_runtime_generic_backend_child(RuntimeMediaBackend *backend);

void update_runtime_generic_backend_child(RuntimeMediaBackend *backend);

int32_t update_runtime_resource_animation_backend(RuntimeMediaBackend *backend);

RuntimeGenericBackend *create_runtime_generic_backend(uintptr_t text_address, uint32_t text_size);

RuntimeGenericBackend *acquire_runtime_generic_backend(void *identity);

void clear_runtime_generic_backend_ready(RuntimeGenericBackend *backend);

void *find_available_runtime_generic_child(uint32_t maximum_end_position);

int32_t find_runtime_generic_text_entry(RuntimeGenericBackend *backend, int32_t category, const char *name);

RuntimeGenericBackendChild *create_runtime_generic_backend_child(void *backend_identity, void *font_identity, const uintptr_t *context, uintptr_t selection, uint32_t flags);

RuntimeGenericBackendChild *acquire_runtime_generic_backend_child(void *identity);

int32_t parse_runtime_generic_integer(const char *text, uint32_t *position, uint32_t end, uint32_t flags);

int32_t skip_runtime_generic_statement(const char *text, uint32_t *position, uint32_t end, uint32_t flags);

int32_t parse_runtime_generic_directive(const char *text, uint32_t *position, uint32_t end, uint32_t flags);

uint32_t build_runtime_generic_backend_child_state(void *identity, uint32_t selection, uint32_t *state, DisplaySceneDescriptor *descriptor, uintptr_t *context);

void publish_runtime_generic_backend_child_state(void *identity, const uint32_t *state, const DisplaySceneDescriptor *descriptor, int32_t end_offset);

uint32_t measure_runtime_font_glyph(uint8_t character, const RuntimeMediaBackend *backend);

uint32_t draw_runtime_font_glyph(DisplaySceneDescriptor *destination, uint8_t character, int32_t x, int32_t y, const RuntimeMediaBackend *font, uint32_t low_color, uint32_t high_color);

void draw_runtime_generic_text(const char *text, uint32_t end, const uint32_t *state, void *font_identity, DisplaySceneDescriptor *destination, uint32_t flags);

uint32_t initialize_runtime_standalone_text(const char *text, uint32_t x, uint32_t y, void *font_identity, uint32_t low_color, uint32_t high_color, RuntimeStandaloneTextState *state);

void draw_runtime_standalone_text(RuntimeStandaloneTextState *state, DisplaySceneDescriptor *destination);

void measure_runtime_generic_text(uint32_t *bounds, const char *text, uint32_t *position, uint32_t end, void *font_identity, uint32_t flags);

uint32_t select_runtime_generic_text(uint32_t *bounds, const char *text, uint32_t *position, uint32_t end, uint32_t search_position, void *font_identity, uint32_t flags);

void release_runtime_generic_backend_child_lock(RuntimeGenericBackendChild *child);

void release_runtime_media_backend_lock(RuntimeMediaBackend *backend);

uint32_t get_runtime_generic_backend_child_flags(void *identity);

void clear_runtime_generic_backend_child_ready(void *identity);

void enable_runtime_generic_backend_child_mode_200(void *identity);

void disable_runtime_generic_backend_child_mode_200(void *identity);

bool get_runtime_generic_backend_child_context(void *identity, uintptr_t *context);

bool set_runtime_generic_backend_child_context(void *identity, const uintptr_t *context);

uint32_t query_runtime_generic_backend_child_state(void *identity, uint32_t *state, DisplaySceneDescriptor *descriptor, uintptr_t *context);

void *destroy_runtime_generic_backend_child(void *identity);

void process_available_runtime_generic_children(uint32_t maximum_end_position);

uint32_t destroy_runtime_generic_backend(void *identity);

} // namespace gag
