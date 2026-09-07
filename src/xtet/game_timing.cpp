#include "game_timing.h"
#include <bit>

namespace xtet
{

uint32_t get_game_tick_interval(uint32_t level)
{
    constexpr uint32_t intervals[]{ 300, 200, 150, 135, 125, 117, 110, 105, 100, 90 };
    return level >= 1 && level <= 10 ? intervals[level - 1] : 500;
}

bool is_game_deadline_pending(uint32_t current_time, uint32_t deadline)
{
    return std::bit_cast<int32_t>(current_time - deadline) < 0;
}

} // namespace xtet
