#pragma once

#include <stdint.h>
#include "../portable_types.h"
#include "../runtime_input.h"

namespace xtet
{

struct PcmFormat;

constexpr uint32_t kDispatchBusy = 0x20000;

enum class HostEventType
{
    TERMINATE,
    PAUSE,
    RESUME,
    RESULT
};

using HostEventCallback = void (*)(HostEventType type, uint32_t result_type, const void *data, uint32_t size);
using InputDrainCallback = void (*)();

using GameHostContext = freegag::RuntimeGameHostContext;

using DirtyRegionCallback = void (*)(int32_t, int32_t, int32_t, int32_t);
using SoundCreateCallback = uint32_t (*)(const PcmFormat *);
using SoundDestroyCallback = void (*)(uint32_t);
using SoundQueueCallback = uint32_t (*)(uint32_t, const void *, uint32_t, int32_t);
using SoundControlCallback = uint32_t (*)(uint32_t, int32_t);
using AnimationDelayCallback = void (*)(uint32_t);

struct GameHostServices
{
    DirtyRegionCallback invalidate_region;
    SoundCreateCallback create_sound;
    SoundDestroyCallback destroy_sound;
    SoundQueueCallback queue_sound;
    SoundControlCallback pause_sound;
    SoundControlCallback resume_sound;
    AnimationDelayCallback delay_animation;
};

void initialize_game(GameHostContext *host_context, const GameHostServices &services, const char *sfs_name);
void set_host_event_callback(HostEventCallback callback);
void set_input_drain_callback(InputDrainCallback callback);
uint32_t dispatch_game_input(const freegag::RuntimeInputEvent &event);
void execute_game_command(uint32_t command);
void shutdown_game();

} // namespace xtet
