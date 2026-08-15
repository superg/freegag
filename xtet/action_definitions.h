#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace xtet
{

struct ActionDefinition
{
    std::array<std::int8_t, 10> values{};
};

bool parse_action_definitions(const std::vector<std::uint8_t> &bytes, std::vector<ActionDefinition> &definitions);

} // namespace xtet
