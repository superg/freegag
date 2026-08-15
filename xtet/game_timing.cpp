#include "game_timing.h"

namespace xtet
{

std::uint32_t get_game_tick_interval(std::uint32_t level)
{
    constexpr std::uint32_t intervals[]{ 300, 200, 150, 135, 125, 117, 110, 105, 100, 90 };
    return level >= 1 && level <= 10 ? intervals[level - 1] : 500;
}

} // namespace xtet
