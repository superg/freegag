#include "action_definitions.h"
#include <array>
#include <charconv>
#include <string_view>

namespace xtet
{
namespace
{

bool parse_line(std::string_view line, std::array<int, 10> &values)
{
    const char *position = line.data();
    const char *end = line.data() + line.size();
    for(int &value : values)
    {
        while(position != end && (*position == ' ' || *position == '\t'))
            ++position;
        if(position == end)
            return false;
        const std::from_chars_result result = std::from_chars(position, end, value);
        if(result.ec != std::errc())
            return false;
        position = result.ptr;
    }
    while(position != end && (*position == ' ' || *position == '\t' || *position == '\r'))
        ++position;
    return position == end;
}

} // namespace

bool parse_action_definitions(const std::vector<std::uint8_t> &bytes, std::vector<ActionDefinition> &definitions)
{
    definitions.clear();
    const std::string_view text((const char *)bytes.data(), bytes.size());
    std::size_t position = 0;
    while(position < text.size())
    {
        const std::size_t newline = text.find('\n', position);
        const std::size_t line_end = newline == std::string_view::npos ? text.size() : newline;
        const std::string_view line = text.substr(position, line_end - position);
        position = newline == std::string_view::npos ? text.size() : newline + 1;
        if(line.empty() || line == "\r")
            continue;

        std::array<int, 10> values{};
        if(!parse_line(line, values))
            return false;
        if(values[0] < 0 || values[0] >= 5 || values[1] < 0 || values[1] >= 10 || values[2] <= -5 || values[2] >= 5 || values[2] == 0)
            continue;
        ActionDefinition definition;
        for(std::size_t index = 0; index < values.size(); ++index)
            definition.values[index] = (std::int8_t)values[index];
        definitions.push_back(definition);
    }
    return true;
}

} // namespace xtet
