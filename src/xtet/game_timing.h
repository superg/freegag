#pragma once

#include <stdint.h>

namespace xtet
{

uint32_t get_game_tick_interval(uint32_t level);
bool is_game_deadline_pending(uint32_t current_time, uint32_t deadline);

} // namespace xtet
