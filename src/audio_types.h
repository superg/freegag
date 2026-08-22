#pragma once

#include "pcm_format.h"
#include "text_types.h"

namespace gag
{
struct RuntimeSoundStatus
{
    uint32_t control_state;
    uint32_t playback_marker;
    uint32_t schedule_marker;
    uint32_t completed;
    uint32_t infinite_loop;
};

} // namespace gag
