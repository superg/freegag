#pragma once

#include <stdint.h>

namespace freegag
{

uint32_t runtime_milliseconds() noexcept;
bool runtime_deadline_pending(uint32_t current_time, uint32_t deadline) noexcept;

} // namespace freegag
