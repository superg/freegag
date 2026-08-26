#pragma once

#include "runtime_input.h"
#include "runtime_types.h"

namespace xtet
{
struct PcmFormat;
}

namespace freegag
{

struct HostXtEtEvent;

uint32_t create_runtime_game_sound(const xtet::PcmFormat *format);

void unload_runtime_game_dll();

bool load_and_initialize_runtime_game_dll(const char *path);

uint32_t stop_runtime_game_dll();

uint32_t pause_runtime_game_dll();

uint32_t resume_runtime_game_dll();

void handle_runtime_xtet_host_event(const HostXtEtEvent &event);

void handle_runtime_input_event(const RuntimeInputEvent &event);

bool should_discard_runtime_keyboard_input(uint64_t timestamp);

void complete_runtime_keyboard_input_drain();

void finish_runtime_keyboard_input_drain();

void use_portable_runtime_input(bool enabled);

void update_runtime_pointer_position(int32_t x, int32_t y);

} // namespace freegag
