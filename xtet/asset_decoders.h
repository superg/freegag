#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace xtet
{

struct PaletteColor
{
    std::uint8_t blue{};
    std::uint8_t green{};
    std::uint8_t red{};
    std::uint8_t reserved{};
};

struct IndexedBitmap
{
    std::uint32_t width{};
    std::uint32_t height{};
    std::array<PaletteColor, 256> palette{};
    std::vector<std::uint8_t> pixels;
};

struct PcmFormat
{
    std::uint16_t format_tag{};
    std::uint16_t channel_count{};
    std::uint32_t samples_per_second{};
    std::uint32_t average_bytes_per_second{};
    std::uint16_t block_alignment{};
    std::uint16_t bits_per_sample{};
};

struct WavePcm
{
    PcmFormat format;
    std::vector<std::uint8_t> samples;
};

static_assert(sizeof(PcmFormat) == 16);

bool decode_indexed_bitmap(const std::vector<std::uint8_t> &bytes, IndexedBitmap &bitmap);
bool decode_wave_pcm(const std::vector<std::uint8_t> &bytes, WavePcm &wave);

} // namespace xtet
