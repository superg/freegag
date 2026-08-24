#pragma once

#include <array>
#include <stddef.h>
#include <stdint.h>
#include "../portable_types.h"
#include "../runtime_input.h"

namespace xtet
{

struct PcmFormat;

constexpr uint32_t kDispatchBusy = 0x20000;
constexpr size_t kCallbackCount = 35;

enum class HostEventType
{
    terminate,
    pause,
    resume,
    result
};

using HostEventCallback = void (*)(HostEventType type, uint32_t result_type, const void *data, uint32_t size);
using InputDrainCallback = void (*)();

struct GameHostContext
{
    uint32_t bits_per_pixel{ 32 };
    uint32_t unknown10{};
    uint16_t width{ 640 };
    uint16_t height{ 480 };
    void *display_surface{};
    intptr_t unknown28{};
    void *framebuffer{};
    intptr_t unknown30{};
    gag::PaletteEntry *palette_entries{};
    uint32_t x_offset{};
    uint32_t y_offset{};
};

using DirtyRegionCallback = void (*)(int32_t, int32_t, int32_t, int32_t);
using SoundCreateCallback = uint32_t (*)(const PcmFormat *);
using SoundDestroyCallback = void (*)(uint32_t);
using SoundQueueCallback = uint32_t (*)(uint32_t, const void *, uint32_t, int32_t);
using SoundControlCallback = uint32_t (*)(uint32_t, int32_t);

void initialize_game(GameHostContext *host_context, void **callback_table, const char *sfs_name);
void set_host_event_callback(HostEventCallback callback);
void set_input_drain_callback(InputDrainCallback callback);
uint32_t dispatch_game_input(const gag::RuntimeInputEvent &event);
void execute_game_command(uint32_t command);
void shutdown_game();

} // namespace xtet
