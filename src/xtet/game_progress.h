#pragma once

#include <functional>
#include <stdint.h>

namespace xtet
{

struct GameProgress
{
    uint32_t score{};
    uint32_t base_level{};
    uint32_t level{};
    uint32_t gameplay_state{};
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
