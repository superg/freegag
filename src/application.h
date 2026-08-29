#pragma once

#include "runtime_types.h"

namespace freegag
{

void dispatch_application_action(ApplicationState *state, ApplicationAction action);

uint32_t application_host_event_type();

bool initialize_graphics_host(int16_t width, uint16_t height);

uint32_t shutdown_graphics_host();

void set_runtime_script_property(ScriptRuntimeProperty property, RuntimeGenericResourceNode *value);

void get_runtime_script_property(ScriptRuntimeProperty property, void **value, void *result);

ApplicationState *initialize_gag_application(int width, int height);

uint32_t initialize_runtime_media_backend();

uint32_t initialize_runtime_generic_backend();

uint32_t shutdown_runtime_generic_backend();

uint32_t initialize_async_file_subsystem();

void set_script_runtime_root_if_valid(ScriptRuntimeRoot *root);

void set_runtime_named_node_enabled(void *identity, int enabled);

void save_runtime_settings(ApplicationState *state);

bool load_saved_window_rectangle(int32_t minimum_width, int32_t minimum_height, PortableRectangle *rectangle);

void save_window_position(ApplicationState *state);

void set_game_cursor_active(ApplicationState *state, int active);

void finish_credits_state(ApplicationState *state, RuntimeTreeNode *tree);

void update_application_window_layout(ApplicationState *state);

void restore_application_display(ApplicationState *state);

void process_state_activation(ApplicationState *state, RuntimeTreeNode *tree);

void finish_application_state_load(ApplicationState *state, const char *path);

bool write_synchronized_cdf_package(void *path, void *comment, void *unused, void *script_state);

void load_local_preferences(ApplicationState *state);

uint32_t shutdown_runtime_media_backend();

int append_string(char *destination, const char *source);

void set_runtime_paths_once(const char *first_path, const char *second_path);

void *capture_save_game_bitmap(uint32_t *size, int half_resolution);

void clear_runtime_display();

void clear_runtime_active_flag(ApplicationState *state);

void clear_application_lock_flag(ApplicationState *state);

void free_heap_memory(void *memory);

} // namespace freegag
