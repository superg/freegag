#pragma once

#include <array>
#include <stdint.h>
#include <vector>

namespace xtet
{

struct PaletteColor
{
    uint8_t blue{};
    uint8_t green{};
    uint8_t red{};
};

struct IndexedBitmap
{
    uint32_t width{};
    uint32_t height{};
    std::array<PaletteColor, 256> palette{};
    std::vector<uint8_t> pixels;
};

struct PcmFormat
{
    uint16_t format_tag{};
    uint16_t channel_count{};
    uint32_t samples_per_second{};
    uint32_t average_bytes_per_second{};
    uint16_t block_alignment{};
    uint16_t bits_per_sample{};
};

struct WavePcm
{
    PcmFormat format;
    std::vector<uint8_t> samples;
};


bool decode_indexed_bitmap(const std::vector<uint8_t> &bytes, IndexedBitmap &bitmap);
bool decode_wave_pcm(const std::vector<uint8_t> &bytes, WavePcm &wave);

} // namespace xtet
