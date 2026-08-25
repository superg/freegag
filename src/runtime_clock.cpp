#include "runtime_clock.h"
#include <chrono>

namespace freegag
{

uint32_t runtime_milliseconds() noexcept
{
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    return static_cast<uint32_t>(milliseconds);
}

} // namespace freegag
