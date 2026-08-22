#pragma once

#include "runtime_types.h"

namespace gag
{

void unload_runtime_game_dll();

bool load_and_initialize_runtime_game_dll(const char *path);

uint32_t stop_runtime_game_dll();

uint32_t pause_runtime_game_dll();

uint32_t resume_runtime_game_dll();

LRESULT CALLBACK runtime_game_window_procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

void update_runtime_pointer_position(int32_t x, int32_t y);

} // namespace gag
