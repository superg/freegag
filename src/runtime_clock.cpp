#include "runtime_clock.h"
#include <bit>
#include <chrono>

namespace freegag
{

uint32_t runtime_milliseconds() noexcept
{
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    return static_cast<uint32_t>(milliseconds);
}

bool runtime_deadline_pending(uint32_t current_time, uint32_t deadline) noexcept
{
    return std::bit_cast<int32_t>(current_time - deadline) < 0;
}

} // namespace freegag
