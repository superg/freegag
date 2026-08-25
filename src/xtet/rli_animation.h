#pragma once

#include <array>
#include <stdint.h>
#include <string>
#include <vector>
#include "asset_decoders.h"
#include "sfs_archive.h"

namespace xtet
{
enum RliFrameFlag : uint16_t
{
    RLI_FRAME_RAW_PIXELS = 0x0001,
    RLI_FRAME_RLE_PIXELS = 0x0002,
    RLI_FRAME_PALETTE = 0x0004
};

struct RliFrameRecord
{
    std::array<uint8_t, 20> bytes{};
    uint16_t flags{};
    uint32_t data_offset{};
    int16_t palette_start{};
    int16_t palette_count{};
    int16_t left{};
    int16_t top{};
    int16_t right{};
    int16_t bottom{};
    std::array<PaletteColor, 256> palette{};
    std::array<uint8_t, 256> palette_defined{};
    std::vector<uint8_t> pixels;
    std::vector<uint8_t> coverage;
};

struct RliAnimation
{
    std::string path;
    int32_t width{};
    int32_t height{};
    std::vector<uint8_t> resident_bytes;
    std::vector<RliFrameRecord> frame_records;
};

bool decode_rli_animation(const std::string &path, const std::vector<uint8_t> &bytes, RliAnimation &animation);
bool load_rli_animations(const SfsArchive &archive, const std::vector<std::string> &paths, std::vector<RliAnimation> &animations);

} // namespace xtet
