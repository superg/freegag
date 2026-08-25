#pragma once

#include "pcm_format.h"
#include "text_types.h"

namespace freegag
{
inline constexpr uint32_t RUNTIME_SOUND_LOOP_INFINITE = 0xffffffff;
inline constexpr uint32_t RUNTIME_SOUND_PLAYBACK_MARKER_PRESET = 0xffffffff;

struct RuntimeSoundStatus
{
    uint32_t control_state;
    uint32_t playback_marker;
    uint32_t schedule_marker;
    uint32_t infinite_loop;
};

} // namespace freegag
