#pragma once

#include "pcm_format.h"
#include "text_types.h"

namespace gag
{
struct RuntimeSoundBufferNode
{
    void *data;
    RuntimeSoundBufferNode *next;
    uint32_t offset;
    uint32_t schedule_offset;
    uint32_t size;
};


struct RuntimeSoundSlot
{
    uint32_t active;
    uint32_t playing;
    uint32_t base_state;
    uint32_t playback_state;
    uint32_t schedule_state;
    uint32_t fade_block_index;
    uint32_t fade_current;
    uint8_t fade_step;
    uint8_t unknown_001d[3];
    uint32_t loop_value_1;
    uint32_t loop_value_2;
    uint8_t volume;
    uint8_t unknown_0029;
    uint16_t conversion_flags;
    uint32_t transition_flags;
    RuntimeSoundBufferNode *buffers;
};


} // namespace gag
