#pragma once

#include <cstdint>
#include <functional>

namespace xtet
{

struct GameProgress
{
    std::uint32_t score{};
    std::uint32_t base_level{};
    std::uint32_t level{};
    std::uint32_t gameplay_state{};
};

struct ProgressUpdate
{
    bool score_changed{};
    bool level_changed{};
    bool game_over{};
};

using ProgressUpdateCallback = std::function<void(const GameProgress &, const ProgressUpdate &)>;

ProgressUpdate update_progress_after_figurine_removal(GameProgress &progress);

} // namespace xtet
