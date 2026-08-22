#pragma once

#include <stdint.h>

namespace gag
{

struct RuntimePcmFormat
{
    uint16_t format_tag;
    uint16_t channel_count;
    uint32_t samples_per_second;
    uint32_t average_bytes_per_second;
    uint16_t block_alignment;
    uint16_t bits_per_sample;
};

} // namespace gag
