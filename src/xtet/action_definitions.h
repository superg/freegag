#pragma once

#include <array>
#include <stdint.h>
#include <vector>

namespace xtet
{

struct ActionDefinition
{
    std::array<int8_t, 10> values{};
};

bool parse_action_definitions(const std::vector<uint8_t> &bytes, std::vector<ActionDefinition> &definitions);

} // namespace xtet
