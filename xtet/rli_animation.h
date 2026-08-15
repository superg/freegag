#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include "sfs_archive.h"

namespace xtet
{

struct RliFrameRecord
{
    std::array<std::uint8_t, 20> bytes{};
    std::uint16_t flags{};
    std::uint32_t data_offset{};
    std::int16_t palette_start{};
    std::int16_t palette_count{};
    std::int16_t left{};
    std::int16_t top{};
    std::int16_t right{};
    std::int16_t bottom{};
    std::vector<std::uint8_t> pixels;
    std::vector<std::uint8_t> coverage;
};

struct RliAnimation
{
    std::string path;
    std::uint16_t flags{};
    std::int32_t width{};
    std::int32_t height{};
    std::vector<std::uint8_t> resident_bytes;
    std::vector<RliFrameRecord> frame_records;
};

bool decode_rli_animation(const std::string &path, const std::vector<std::uint8_t> &bytes, RliAnimation &animation);
bool load_rli_animations(const SfsArchive &archive, const std::vector<std::string> &paths, std::vector<RliAnimation> &animations);

} // namespace xtet
