#pragma once

#include "runtime_types.h"

namespace gag
{

int validate_startup_environment(ApplicationState *state, const char *requested_archive, uint32_t stages);

bool register_gag_window_classes(ApplicationState *state);

GraphicsHostInitializationResult *initialize_graphics_host(HINSTANCE instance, HWND parent, int x, int y, int16_t width, uint16_t height, uint32_t flags);

uint32_t shutdown_graphics_host();

void set_runtime_script_property(uint32_t property, void *context, void *value);

void get_runtime_script_property(uint32_t property, void **value, void *result);

ApplicationState *initialize_gag_application(int width, int height, HINSTANCE instance, bool start_xtet, int show_command);

uint32_t initialize_runtime_media_backend();

uint32_t initialize_runtime_generic_backend();

uint32_t shutdown_runtime_generic_backend();

uint32_t initialize_async_file_subsystem();

void set_script_runtime_root_if_valid(ScriptRuntimeRoot *root);

void set_runtime_named_node_enabled(void *identity, int enabled);

LRESULT CALLBACK gag_main_window_procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

LRESULT CALLBACK gag_capture_window_procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

void save_runtime_settings(ApplicationState *state);

bool load_saved_window_position(int32_t width, int32_t height, POINT *position);

bool load_saved_window_rectangle(int32_t minimum_width, int32_t minimum_height, RECT *rectangle);

void save_window_position(ApplicationState *state);

void set_game_cursor_active(ApplicationState *state, int active);

void finish_credits_state(ApplicationState *state, RuntimeTreeNode *tree);

void update_modern_windows_windowed_viewport(ApplicationState *state);

void update_application_window_layout(ApplicationState *state, SecondaryWindowLayout *secondary_layout);

void restore_application_display(ApplicationState *state);

void process_state_activation(ApplicationState *state, RuntimeTreeNode *tree);

void finish_application_state_load(ApplicationState *state, const char *path);

bool write_synchronized_cdf_package(void *path, void *comment, void *unused, void *script_state);

uint32_t load_local_preferences(ApplicationState *state);

RuntimeMediaBackend *acquire_first_runtime_media_backend();

uint32_t shutdown_runtime_media_backend();

int append_string(char *destination, const char *source);

uint32_t enable_borderless_fullscreen(ApplicationState *state);

void set_runtime_paths_once(const char *first_path, const char *second_path);

void *capture_save_game_bitmap(void *game_context, uint32_t *size, int half_resolution);

void clear_credits_runtime_flag();

void clear_runtime_display();

void set_application_lock_flag(ApplicationState *state);

void set_application_inactive_flags(ApplicationState *state);

void clear_runtime_active_flag(ApplicationState *state);

void clear_application_lock_flag(ApplicationState *state);

void free_heap_memory(void *memory);

} // namespace gag
